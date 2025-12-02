#include <esp_now.h>
#include <WiFi.h>
#include <Elog.h>
#include "math.hpp"

#define DEBUG_LOG 0
#define INFO_LOG 1
#define ERR_LOG 2

long minDist1, minDist2;
//QueueSetHandle_t QDistance1 = NULL;
//QueueSetHandle_t QDistance2 = NULL;

long Distance1, Distance2;

long maxRange = 7.0;

TaskHandle_t UltraTaskHandle = NULL;
TaskHandle_t WheelsTaskHandle = NULL;

struct Message {
  int x;
  int y;
};

struct UltraSound
{
    int trigPin;
    int echoPin;

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
        
        duration = pulseIn(echoPin, HIGH);
        
        // Convert the time into a distance
        cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
        
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Distance Measurement: %d", cm);
        //Serial.println(cm);
        return cm;
    }
};

struct Hbro
{
  int motorA1;
  int motorA2;
  int motorB1;
  int motorB2;

    void setupPins() 
    {
        pinMode(motorA1, OUTPUT);
        pinMode(motorA2, OUTPUT);
        pinMode(motorB1, OUTPUT);
        pinMode(motorB2, OUTPUT);
    }
    void forward(int speed)
    {
        analogWrite(motorA1, speed);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, speed);
        analogWrite(motorB2, 0);
    }
    void backward(int speed)
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, speed);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, speed);
    }
    void right(int speed)
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, speed);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
    }
    void left(int speed)
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, speed);
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

Message messageData;

// Commented for now will be used in the future
// callback function that will be executed when data is received
/*void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  Serial.print("Bytes received: ");
  Serial.print("X Value: ");
  Serial.println(messageData.x);
  Serial.print("Y Value: ");
  Serial.println(messageData.y);
  Serial.println();
}
*/

void UltraTask(void *parameter) {
    long qDist1, qDist2;

    while(true){
        Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Distance1...");
        qDist1 = leftU.distance();
        Distance1 = qDist1;
        //xQueueSend(QDistance1, &qDist1, portMAX_DELAY);

        Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Distance2...");
        qDist2 = rightU.distance();
        Distance2 = qDist2;
        //xQueueSend(QDistance2, &qDist2, portMAX_DELAY);

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void WheelsTask(void *parameter) {
    while(true){
        //FIX add handling for when getting data from queue is taking too long
        //xQueueReceive(QDistance1, &range1, portMAX_DELAY);
        //xQueueReceive(QDistance2, &range2, portMAX_DELAY);
        long range1 = Distance1;
        long range2 = Distance2;

        if (range1 < maxRange)
        {
            Serial.println(range1);
            //TODO swap print for Logger
            Serial.println("Too close right");
            frontWheels.left(100);
            backWheels.right(100);
        }
        else if(range2 < maxRange)
        {
            Serial.println(range2);
            //TODO swap print for Logger
            Serial.println("Too close left");
            frontWheels.right(100);
            frontWheels.left(100);
        }
        else
        {
            //TODO swap print for Logger
            Serial.println("Moving forward");
            frontWheels.forward(255);
            backWheels.forward(255);
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void setup() {
  Serial.begin(9600);

  // Logger for debugging and more
  Logger.registerSerial(DEBUG_LOG, ELOG_LEVEL_DEBUG, "test"); // We want messages with DEBUG level and lower
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Logger.log(ERR_LOG, ELOG_LEVEL_ERROR, "Error initializing ESP-NOW");
    //Serial.println("Error initializing ESP-NOW");
    return;
  }
  //QDistance1 = xQueueCreate(5, sizeof(long));
  //QDistance2 = xQueueCreate(5, sizeof(long));

  frontWheels.motorA1 = 25;
  frontWheels.motorA2 = 26;
  frontWheels.motorB1 = 32;
  frontWheels.motorB2 = 33;
  frontWheels.setupPins();

  backWheels.motorA1 = 4;
  backWheels.motorA2 = 16;
  backWheels.motorB1 = 17;
  backWheels.motorB2 = 5;
  backWheels.setupPins();

  leftU.trigPin = 19;
  leftU.echoPin = 18;
  leftU.setupPins();

  rightU.trigPin = 13;
  rightU.echoPin = 12;
  rightU.setupPins();

  delay(100);

  // ESP_NOW runs on CORE_0
  //esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  
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