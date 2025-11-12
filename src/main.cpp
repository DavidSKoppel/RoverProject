#include <Arduino.h>

//long minDist1, minDist2;
QueueSetHandle_t QDistance1 = NULL;
QueueSetHandle_t QDistance2 = NULL;
long maxRange = 7.0;

TaskHandle_t UltraTaskHandle = NULL;
TaskHandle_t WheelsTaskHandle = NULL;

struct UltraSound
{
    int trigPin = 12;
    int echoPin = 13;

    long cm, duration;

    void setupPins() 
    {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    long distance(){
        digitalWrite(trigPin, LOW);
        delayMicroseconds(5);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        pinMode(echoPin, INPUT);
        duration = pulseIn(echoPin, HIGH);
        
        // Convert the time into a distance
        cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
        return cm;
    }
};

struct Hbro
{
  int motorA1 = 25;
  int motorA2 = 26;
  int motorB1 = 33;
  int motorB2 = 32;

    void setupPins() 
    {
        pinMode(motorA1, OUTPUT);
        pinMode(motorA2, OUTPUT);
        pinMode(motorB1, OUTPUT);
        pinMode(motorB2, OUTPUT);
    }
    void forward()
    {
        analogWrite(motorA1, 100);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 100);
        analogWrite(motorB2, 0);
    }
    void backward()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 255);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 255);
    }
    void right()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 255);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
    }
    void left()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 255);
    }
    void gentleRight()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 128);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
    }
    void gentleLeft()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 128);
    }
    void stop()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
    }
};

Hbro frontWheels;
Hbro backWheels;

UltraSound leftU;
UltraSound rightU;

void UltraTask(void *parameter) {
    long qDist1, qDist2;

    while(true){
        qDist1 = leftU.distance();
        xQueueSend(QDistance1, &qDist1, portMAX_DELAY);

        qDist2 = rightU.distance();
        xQueueSend(QDistance2, &qDist2, portMAX_DELAY);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void WheelsTask(void *parameter) {
    long range1, range2;

    while(true){
        //FIX add handling for when getting data from queue is taking too long
        xQueueReceive(QDistance1, &range1, portMAX_DELAY);
        xQueueReceive(QDistance2, &range2, portMAX_DELAY);
        
        if (range1 < maxRange)
        {
            Serial.println(range1);
            Serial.println("Too close right");
            frontWheels.gentleLeft();
            backWheels.gentleLeft();
        }
        else if(range2 < maxRange)
        {
            Serial.println(range2);
            Serial.println("Too close left");
            frontWheels.gentleRight();
            backWheels.gentleRight();
        }
        else
        {
            Serial.println("Moving forward");
            frontWheels.forward();
            backWheels.forward();
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void setup() {
  Serial.begin(9600);
  
  frontWheels.motorA1 = 25;
  frontWheels.motorA2 = 26;
  frontWheels.motorB1 = 33;
  frontWheels.motorB2 = 32;
  frontWheels.setupPins();

  backWheels.motorA1 = 4;
  backWheels.motorA2 = 16;
  backWheels.motorB1 = 17;
  backWheels.motorB2 = 5;
  backWheels.setupPins();

  leftU.trigPin = 13;
  leftU.echoPin = 12;
  leftU.setupPins();  

  rightU.trigPin = 19;
  rightU.echoPin = 18;
  rightU.setupPins();

  QDistance1 = xQueueCreate(5, sizeof(long));
  QDistance2 = xQueueCreate(5, sizeof(long));

  xTaskCreatePinnedToCore(
    UltraTask,        // Task function
    "UltraTask",      // Task name
    2048,             // Stack size (bytes)
    NULL,             // Parameters
    1,                // Priority
    &UltraTaskHandle, // Task handle
    0                 // Core
  );

  xTaskCreatePinnedToCore(
    WheelsTask,        // Task function
    "WheelsTask",      // Task name
    2048,              // Stack size (bytes)
    NULL,              // Parameters
    1,                 // Priority
    &WheelsTaskHandle, // Task handle
    1                  // Core
  );
}

void loop() {
}

//Nedenstående er antagelser af hvilke porte skal tændes for at kunne gøre som beskrevet
/*
A1: 1 A2: 0 left forward
A1: 0 A2: 1 left tilbage

B1: 1 B2: 0 højre forward
B1: 0 B2: 1 højre tilbage
*/