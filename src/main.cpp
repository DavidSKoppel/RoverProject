#include <esp_now.h>
#include <WiFi.h>
#include <Elog.h>
#include "math.hpp"

#define DEBUG_LOG 0
#define INFO_LOG 1
#define ERR_LOG 2

TaskHandle_t WheelsTaskHandle = NULL;

struct Message {
  int x;
  int y;
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

Message messageData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  Serial.print("Bytes received: ");
  Serial.print("X Value: ");
  Serial.println(messageData.x);
  Serial.print("Y Value: ");
  Serial.println(messageData.y);
  Serial.println();
}

void WheelsTask(void *parameter) {
    long rangeX, rangeY;

    while(true){
        rangeX = messageData.x;
        rangeY = messageData.y;

        float speedF = map(rangeX, 2900.0, 4095.0, 0.0, 255.0);
        float speedB = map(rangeX, 2900.0, 0.0, 0.0, 255.0);
        
        if(speedF > speedB)
        {
            frontWheels.forward(speedF);
        }
        else if (speedF < speedB)
        {
            frontWheels.backward(speedB);
        }
        else
        {
            frontWheels.stop();
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

  frontWheels.motorA1 = 25;
  frontWheels.motorA2 = 26;
  frontWheels.motorB1 = 32;
  frontWheels.motorB2 = 33;
  frontWheels.setupPins();
/*
  backWheels.motorA1 = 4;
  backWheels.motorA2 = 16;
  backWheels.motorB1 = 17;
  backWheels.motorB2 = 5;
  backWheels.setupPins();
*/

  delay(100);

  // ESP_NOW runs on CORE_0
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  

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