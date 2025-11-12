#include <Arduino.h>

//long minDist1, minDist2;
QueueSetHandle_t QDistance1 = NULL, QDistance2 = NULL;
long maxRange = 7.0;

TaskHandle_t UltraTaskHandle = NULL;
TaskHandle_t WheelsTaskHandle = NULL;

struct UltraSound
{
    int trigPin = 12;
    int echoPin = 13;

    long cm, duration;

    QueueSetHandle_t QDistance;

    void setupPins() 
    {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    void distance(){
        digitalWrite(trigPin, LOW);
        delayMicroseconds(5);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        pinMode(echoPin, INPUT);
        duration = pulseIn(echoPin, HIGH);
        
        // Convert the time into a distance
        cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
        xQueueSend(QDistance, &cm, portMAX_DELAY);
        //return cm;
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
        analogWrite(motorA1, 255);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 255);
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
  while(true){
    leftU.distance();
    rightU.distance();
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
            Serial.println("Too close right");
            frontWheels.gentleLeft();
            backWheels.gentleLeft();
        }
        else if(range2 < maxRange)
        {
            Serial.println("Too close left");
            frontWheels.gentleRight();
            backWheels.gentleRight();
        }
        else
        {
            frontWheels.forward();
            backWheels.forward();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
  backWheels.motorB1 = 5;
  backWheels.motorB2 = 17;
  backWheels.setupPins();

  leftU.trigPin = 13;
  leftU.echoPin = 12;
  leftU.QDistance = QDistance1;
  leftU.setupPins();  

  rightU.trigPin = 19;
  rightU.echoPin = 18;
  leftU.QDistance = QDistance2;
  rightU.setupPins();

  xTaskCreatePinnedToCore(
    UltraTask,        // Task function
    "UltraTask",      // Task name
    4096,             // Stack size (bytes)
    NULL,             // Parameters
    1,                // Priority
    &UltraTaskHandle, // Task handle
    0                 // Core
  );

  xTaskCreatePinnedToCore(
    WheelsTask,        // Task function
    "WheelsTask",      // Task name
    4096,              // Stack size (bytes)
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