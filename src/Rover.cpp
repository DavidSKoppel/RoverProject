#include <esp_now.h>
#include <WiFi.h>

#include "Wheels.hpp"
#include "Arm.hpp"
#include "UltraSound.hpp"

enum Mode {
    AUTO = 0,
    DRIVE = 1,
    ARM = 2,
};

Mode currentMode = DRIVE;
//May be used later for handling changing states fluidly
//bool canChangeState = true;

//The Ultrasound distances
long Distance1, Distance2;
long maxRange = 7.0;

//Message received from controller, 1900 is the standard resting point of its joysticks potentiometers
struct Message {
  int x = 1900;
  int y = 1900;
  int state = 0;
};

//Instantiate all different Tasks
TaskHandle_t WheelsTaskHandle = NULL;
TaskHandle_t ArmTaskHandle = NULL;
TaskHandle_t UltraTaskHandle = NULL;

//Instantiate all minor components and messagedata
Message messageData;

//These structs comes with contructors that automatically sets the GPIO, instead of writing leftU.trigpin = 19, and so on
UltraSound leftU(19,18);
UltraSound rightU(13,12);

Hbro frontWheels(25,26,32,33);
Hbro backWheels(4,16,5,17);

void WheelsTask(void *parameter) {
  double rangeX, rangeY;

  while(true){
    if(currentMode == DRIVE){
      rangeX = messageData.x;
      rangeY = messageData.y;

      float speedB = map(rangeX, 2300.0, 4095.0, 0.0, 255.0);
      float speedF = map(rangeX, 1800.0, 0.0, 0.0, 255.0);

      float speedR = map(rangeY, 2300.0, 4095.0, 0.0, 255.0);
      float speedL = map(rangeY, 1800.0, 0.0, 0.0, 255.0);

      if(speedF < 200)
        speedF = 0;
      if(speedB < 200)
        speedB = 0;
      if(speedR < 200)
        speedR = 0;
      if(speedL < 200)
        speedL = 0;

      if(speedF > speedB)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Driving %f", speedF);
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
    else if (currentMode == AUTO)
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
    if(currentMode == AUTO){
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
  double armX, armY;
  while (true)
  {
    if(currentMode == AUTO){
      //Test script for the arm
      setPWMOverTime(SERVO_BASE, 100,180,1000);
      setPWMOverTime(SERVO_BASE, 180,10,1000);
      setPWMOverTime(SERVO_BASE, 0,100,1000);

      setPWMOverTime(SERVO_ARM_R, 0,90,1000);

      setPWMOverTime(SERVO_ARM_CLAW, 10,90,1000);
      setPWMOverTime(SERVO_ARM_CLAW, 90,10,1000);

      setPWMOverTime(SERVO_ARM_R, 90,0,1000);
      vTaskDelay(250 / portTICK_PERIOD_MS);
    } else if (currentMode == ARM){
      armX = messageData.x;
      armY = messageData.y;

      float armBPos = map(armY, 0.0, 4095.0, 0.0, 180.0);

      PWMBoard.setPWM(SERVO_BASE, 0, armBPos);

    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

//Callback function that will be executed when data is received
void DataReceivedTask(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  if (messageData.state == 2){
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Changing state");
    if (currentMode == ARM)
        currentMode = AUTO;
    else
        currentMode = static_cast<Mode>(currentMode + 1);
  } else if (messageData.state == 1){
    // TODO handle single button press
  }
  messageData.state = 0;
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Bytes received:");
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "X-value: %d", messageData.x);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Y-value: %d", messageData.y);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "State: %d", messageData.state);
}

void setup() {
  Serial.begin(115200);

  //Logger for debugging and more
  Logger.registerSerial(DEBUG_LOG, ELOG_LEVEL_DEBUG, "test"); //We want messages with DEBUG level and lower
  
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing WiFi");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Logger.log(ERR_LOG, ELOG_LEVEL_ERROR, "Error initializing ESP-NOW");
    return;
  }

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing PWMBoard");
  while(!PWMBoard.begin()){
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, ".");
    delay(1);
  }
  PWMBoard.setPWMFreq(50);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Continuing"); //Major checkpoint, past this are minor components startup
  
  frontWheels.setupPins();
  backWheels.setupPins();

  //leftU.setupPins();
  //rightU.setupPins();

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