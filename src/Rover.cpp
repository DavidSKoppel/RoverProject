#include <esp_now.h>
#include <WiFi.h>

//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

#include "Wheels.hpp"
#include "Arm.hpp"
#include "UltraSound.hpp"

enum Mode {
    AUTO = 0,
    DRIVE = 1,
    ARM = 2,
};

Mode currentMode = ARM;

//May be used later for handling changing states fluidly
//bool canChangeState = true;

//The Ultrasound distances
long Distance1 = 100; 
long Distance2 = 100;
long Distance3 = 100;
long maxRange = 10.0;

//Message received from controller, 1900 is the standard resting point of its joysticks potentiometers
struct Message {
  int RX = 1900;
  int RY = 1900;
  int LX = 1900;
  int LY = 1900;
  int Rbutton = 0;
  int Lbutton = 0;
};

//Instantiate all different Tasks
TaskHandle_t WheelsTaskHandle = NULL;
TaskHandle_t ArmTaskHandle = NULL;
TaskHandle_t UltraTaskHandle = NULL;
TaskHandle_t BuzzerTaskHandle = NULL;

//Instantiate all minor components and messagedata
Message messageData;

//These structs comes with contructors that automatically sets the GPIO, instead of writing leftU.trigpin = 19, and so on
UltraSound leftU(19,18);
UltraSound rightU(13,23);
//UltraSound middleU(13,12);

Hbro frontWheels(25,26,32,33);
Hbro backWheels(5,17,4,16);

void WheelsTask(void *parameter) {
  double rangeX, rangeY;

  while(true){
    if(currentMode == DRIVE){
      rangeX = messageData.RX;
      rangeY = messageData.RY;

      float speedB = map(rangeX, 2300.0, 4095.0, 0.0, 255.0);
      float speedF = map(rangeX, 1800.0, 0.0, 0.0, 255.0);

      float speedL = map(rangeY, 2300.0, 4095.0, 0.0, 255.0);
      float speedR = map(rangeY, 1800.0, 0.0, 0.0, 255.0);

      if(speedF < 200)
        speedF = 0;
      if(speedB < 200)
        speedB = 0;
      if(speedR < 200)
        speedR = 0;
      if(speedL < 190)
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
      vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    else if (currentMode == AUTO)
    {
      long range1 = Distance1;
      long range2 = Distance2;

      if (range1 < maxRange)
      {
          Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close left");
          frontWheels.right(200);
          backWheels.right(200);
      }
      else if(range2 < maxRange)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close right");
        frontWheels.left(200);
        backWheels.left(200);
      }
      else
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Moving forward");
        frontWheels.forward(200);
        backWheels.forward(200);
      }
      vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void UltraTask(void *parameter) {
  long qDist1, qDist2, qDist3;

  while(true){
    if(currentMode == AUTO){
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Left Distance...");
      qDist1 = leftU.distance();
      Distance1 = qDist1;
      //xQueueSend(QDistance1, &qDist1, portMAX_DELAY);

      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Right Distance...");
      qDist2 = rightU.distance();
      Distance2 = qDist2;
      //xQueueSend(QDistance2, &qDist2, portMAX_DELAY);
/*
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Reading Middle Distance...");
      qDist3 = middleU.distance();
      Distance3 = qDist3;
*/
      vTaskDelay(30 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void ArmTask(void *parameter) {
  bool isTurnedOff = false;
  int BaseData, ArmBData, ArmCData, ClawData;
  int servoBasePos = 300;
  int servoArmBPos = 400;
  int servoArmCPos = 400;
  int servoClawPos = 470;

  //Startup for resetting position
  PWMBoard.setPWM(SERVO_BASE, 0, servoBasePos);
  PWMBoard.setPWM(SERVO_ARM_B, 0, servoArmBPos);
  PWMBoard.setPWM(SERVO_ARM_C, 0, servoArmCPos);
  PWMBoard.setPWM(SERVO_CLAW, 0, servoClawPos);

  while (true)
  {
    if (currentMode == ARM){
      isTurnedOff = false;
      BaseData = messageData.LY;
      ArmBData = messageData.LX;
      ArmCData = messageData.RX;
      ClawData = messageData.RY;

      // if BaseData = 4095 then result is -3095
      // if BaseData = 0 then result is 1900
      int baseSpeed = 1900-BaseData;
      int armBSpeed = 1900-ArmBData;
      int armCSpeed = 1900-ArmCData;
      int clawSpeed = 1900-ClawData;

      int newPos1 = baseSpeed * 0.00053;
      servoBasePos += newPos1;
      servoBasePos = (servoBasePos < 100) ? 100 : servoBasePos;
      servoBasePos = (servoBasePos > 670) ? 670 : servoBasePos;

      
      int newPos2 = armBSpeed * 0.00053;
      servoArmBPos += newPos2;
      servoArmBPos = (servoArmBPos < 300) ? 300 : servoArmBPos;
      servoArmBPos = (servoArmBPos > 670) ? 670 : servoArmBPos;

      
      int newPos3 = armCSpeed * 0.00053;
      servoArmCPos += newPos3;
      servoArmCPos = (servoArmCPos < 100) ? 100 : servoArmCPos;
      servoArmCPos = (servoArmCPos > 670) ? 670 : servoArmCPos;

      
      int newPos4 = clawSpeed * 0.00053;
      servoClawPos += newPos4;
      servoClawPos = (servoClawPos < 470) ? 470 : servoClawPos;
      servoClawPos = (servoClawPos > 670) ? 670 : servoClawPos;

      PWMBoard.setPWM(SERVO_CLAW, 0, servoClawPos);
      PWMBoard.setPWM(SERVO_ARM_C, 0, servoArmCPos);
      PWMBoard.setPWM(SERVO_ARM_B, 0, servoArmBPos);
      PWMBoard.setPWM(SERVO_BASE, 0, servoBasePos);
      
      //setServoPos(SERVO_ARM_B,armBSpeed,servoArmBPos);

      vTaskDelay(6 / portTICK_PERIOD_MS);
    } 
    else 
    {
      if (!isTurnedOff){
        PWMBoard.setPWM(SERVO_BASE, 0, 4096);
        PWMBoard.setPWM(SERVO_ARM_B, 0, 4096);
        PWMBoard.setPWM(SERVO_ARM_C, 0, 4096);
        PWMBoard.setPWM(SERVO_CLAW, 0, 4096);
        isTurnedOff = true;
      }
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void BuzzerTask(void *parameter){
  //tone(pinforbuzzer, 1000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  //noTone(pinforbuzzer);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

//Callback function that will be executed when data is received
void DataReceivedTask(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  if (messageData.Rbutton == 1){
    if (currentMode == ARM)
      currentMode = AUTO;
    else
      currentMode = static_cast<Mode>(currentMode + 1);
    messageData.Rbutton = 0;
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Changing state to: %d", currentMode);
  } else if (messageData.Lbutton == 1){
    if (currentMode == AUTO)
      currentMode = ARM;
    else
      currentMode = static_cast<Mode>(currentMode - 1);
    messageData.Lbutton = 0;
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Changing state to: %d", currentMode);
  }

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Bytes received:");
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "RX-value: %d", messageData.RX);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "RY-value: %d", messageData.RY);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "LX-value: %d", messageData.LX);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "LY-value: %d", messageData.LY);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Right state: %i", messageData.Rbutton);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Left state: %i", messageData.Lbutton);
}

void setup() {
  Serial.begin(115200);
  //Logger for debugging and more
  Logger.registerSerial(INFO_LOG, ELOG_LEVEL_INFO, "teste"); //We want messages with DEBUG level and lower
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
    delay(100);
  }
  PWMBoard.setPWMFreq(50);
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Continuing"); //Major checkpoint, past this are minor components startup
  
  
  frontWheels.setupPins();
  backWheels.setupPins();

  leftU.setupPins();
  rightU.setupPins();

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing DataTask");
  // ESP_NOW runs on CORE_0
  esp_now_register_recv_cb(esp_now_recv_cb_t(DataReceivedTask));

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing UltraTask");
  xTaskCreatePinnedToCore(
    UltraTask,        // Task function
    "UltraTask",      // Task name
    2048,             // Stack size (bytes)
    NULL,             // Parameters
    1,                // Priority
    &UltraTaskHandle, // Task handle
    0                 // Core
  );

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing ArmTask");
  xTaskCreatePinnedToCore(
    ArmTask,        // Task function
    "ArmTask",      // Task name
    2048,           // Stack size (bytes)
    NULL,           // Parameters
    2,              // Priority
    &ArmTaskHandle, // Task handle
    0               // Core
  );


  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing WheelsTask");
  xTaskCreatePinnedToCore(
    WheelsTask,        // Task function
    "WheelsTask",      // Task name
    2048,              // Stack size (bytes)
    NULL,              // Parameters
    1,                 // Priority
    &WheelsTaskHandle, // Task handle
    1                  // Core
  );
/*
  xTaskCreatePinnedToCore(
    BuzzerTask,        // Task function
    "BuzzerTask",      // Task name
    2048,              // Stack size (bytes)
    NULL,              // Parameters
    2,                 // Priority
    &BuzzerTaskHandle, // Task handle
    1                  // Core
  );
  */
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