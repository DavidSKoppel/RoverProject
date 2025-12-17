#include <esp_now.h>
#include <WiFi.h>

#include "Wheels.hpp"
#include "Arm.hpp"
#include "UltraSound.hpp"

bool IsAuto = false;
bool canChangeState = true;

long Distance1, Distance2;

long maxRange = 7.0;

struct Message {
  int x;
  int y;
  int state;
};

Message messageData;

TaskHandle_t WheelsTaskHandle = NULL;
TaskHandle_t ArmTaskHandle = NULL;
TaskHandle_t UltraTaskHandle = NULL;

UltraSound leftU(19,18);
UltraSound rightU(13,12);

Hbro frontWheels(25,26,32,33);
Hbro backWheels(4,16,17,5);

void WheelsTask(void *parameter) {
  double rangeX, rangeY;

  while(true){
    if(!IsAuto){
      rangeX = messageData.x;
      rangeY = messageData.y;

      float speedF = map(rangeX, 2300.0, 4095.0, 0.0, 255.0);
      float speedB = map(rangeX, 1900.0, 0.0, 0.0, 255.0);

      float speedR = map(rangeY, 2300.0, 4095.0, 0.0, 255.0);
      float speedL = map(rangeY, 1900.0, 0.0, 0.0, 255.0);

      if(speedF < 0)
        speedF = 0;
      if(speedB < 0)
        speedB = 0;
      if(speedR < 0)
        speedR = 0;
      if(speedL < 0)
        speedL = 0;

      if(speedF > speedB)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Driving fowards");
          frontWheels.forward(speedF);
          backWheels.forward(speedF);
      }
      else if (speedF < speedB)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Driving backwards");
          frontWheels.backward(speedB);
          backWheels.backward(speedB);
      }
      else if(speedR > speedL){
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Driving right");
        frontWheels.right(speedR);
        backWheels.right(speedR);
      }
      else if(speedR < speedL){
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Driving left");
        frontWheels.left(speedL);
        backWheels.left(speedL);
      }
      else
      {
          frontWheels.stop();
          backWheels.stop();
      }
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    else
    {
      long range1 = Distance1;
      long range2 = Distance2;

      if (range1 < maxRange)
      {
          Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close right");
          frontWheels.left(255);
          backWheels.right(255);
      }
      else if(range2 < maxRange)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close left");
        frontWheels.right(255);
        frontWheels.left(255);
      }
      else
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Moving forward");
        frontWheels.forward(255);
        backWheels.forward(255);
      }
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void UltraTask(void *parameter) {
  long qDist1, qDist2;

  while(true){
    if(IsAuto){
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Distance1...");
      qDist1 = leftU.distance();
      Distance1 = qDist1;
      //xQueueSend(QDistance1, &qDist1, portMAX_DELAY);

      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Distance2...");
      qDist2 = rightU.distance();
      Distance2 = qDist2;
      //xQueueSend(QDistance2, &qDist2, portMAX_DELAY);

      vTaskDelay(250 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void ArmTask(void *parameter) {
  while (true)
  {
    if(IsAuto){
      int rangeX = messageData.x;
      int rangeY = messageData.y;
      float basePos = map(rangeX, 2900.0, 4095.0, 0.0, 255.0);
      float armPos = map(rangeY, 2900.0, 0.0, 0.0, 255.0);

      setPWMOverTime(servoBase, 90,180,1000);
      setPWMOverTime(servoBase, 180,0,1000);
      setPWMOverTime(servoBase, 0,90,1000);

      setPWMOverTime(servoArmR, 0,40,1000);

      setPWMOverTime(servoClaw, 20,90,1000);
      setPWMOverTime(servoClaw, 90,20,1000);

      setPWMOverTime(servoArmR, 40,0,1000);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

//Callback function that will be executed when data is received
void DataReceivedTask(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  if (messageData.state == 1 && canChangeState){
    // TODO handle if controller sends true continuosly
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Changing state");
    IsAuto = !IsAuto;
    canChangeState = false;
    //delay(1000);
  } else if (!messageData.state) {
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Ready to change state again");
    canChangeState = true;
  }
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Bytes received:");
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "X-value: %d", messageData.x);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Y-value: %d", messageData.y);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "State: %d", messageData.state);
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
    2,              // Priority
    &ArmTaskHandle, // Task handle
    0               // Core
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