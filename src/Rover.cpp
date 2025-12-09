#include <esp_now.h>
#include <WiFi.h>

#include "Wheels.hpp"
#include "Arm.hpp"
#include "UltraSound.hpp"

long minDist1, minDist2;

long Distance1, Distance2;

long maxRange = 7.0;

struct Message {
  int x;
  int y;
};

Message messageData;

TaskHandle_t WheelsTaskHandle = NULL;
TaskHandle_t ArmTaskHandle = NULL;
TaskHandle_t UltraTaskHandle = NULL;

UltraSound leftU(19,18);
UltraSound rightU(13,12);

Hbro frontWheels(25,26,32,33);
Hbro backWheels(4,16,17,5);
/*
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
*/

// TODO Be able to work on auto
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

void ArmTask(void *parameter) {
    setPWMOverTime(servoBase, 90,180,1000);
    setPWMOverTime(servoBase, 180,0,1000);
    setPWMOverTime(servoBase, 0,90,1000);

    setPWMOverTime(servoArmR, 0,40,1000);

    setPWMOverTime(servoClaw, 20,90,1000);
    setPWMOverTime(servoClaw, 90,20,1000);

    setPWMOverTime(servoArmR, 40,0,1000);
}

// callback function that will be executed when data is received
void DataReceivedTask(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Bytes received: ");
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "X-value: %d", messageData.x);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Y-value: %d", messageData.y);
}

void setup() {
  Serial.begin(115200);

  //Logger for debugging and more
  Logger.registerSerial(DEBUG_LOG, ELOG_LEVEL_DEBUG, "test"); //We want messages with DEBUG level and lower
  
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing PWMBoard");
  while(!PWMBoard.begin()){
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, ".");
    delay(1);
  }
  PWMBoard.setPWMFreq(50);
  
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing WiFi");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Logger.log(ERR_LOG, ELOG_LEVEL_ERROR, "Error initializing ESP-NOW");
    return;
  }

  frontWheels.setupPins();
  backWheels.setupPins();

  leftU.setupPins();
  rightU.setupPins();

  // ESP_NOW runs on CORE_0
  esp_now_register_recv_cb(esp_now_recv_cb_t(DataReceivedTask));

  xTaskCreatePinnedToCore(
    WheelsTask,        // Task function
    "WheelsTask",      // Task name
    2048,              // Stack size (bytes)
    NULL,              // Parameters
    0,                 // Priority
    &WheelsTaskHandle, // Task handle
    1                  // Core
  );

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
    ArmTask,        // Task function
    "ArmTask",      // Task name
    2048,           // Stack size (bytes)
    NULL,           // Parameters
    0,              // Priority
    &ArmTaskHandle, // Task handle
    0               // Core
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