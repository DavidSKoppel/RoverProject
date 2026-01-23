#include <esp_now.h>
#include <WiFi.h>

//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

#include "Wheels.hpp"
#include "Arm.hpp"
#include "UltraSound.hpp"
#include "ToF.hpp"

enum Mode {
  AUTO = 0,
  DEMO = 1,
  DRIVE = 2,
  ARM = 3,
};

Mode currentMode = DRIVE;

//May be used later for handling changing states fluidly
//bool canChangeState = true;

//Variables for demo mode
bool useArm = false;
bool useWheels = true;

//The Ultrasound distances (centimeters)
long UltraDist1 = 100; 
long UltraDist2 = 100;
long minUltraDist = 10.0;

//Time of Flight laser distance (millimeters)
long ToFDist = 1000;
long minToFDist = 100; 

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
TaskHandle_t ToFTaskHandle = NULL;

//Instantiate all minor components and messagedata
Message messageData;

//These structs comes with contructors that automatically sets the GPIO, instead of writing leftU.trigpin = 19, and so on
UltraSound leftU(19,18);
UltraSound rightU(13,23);
//UltraSound middleU(13,12);

Hbro frontWheels(25,26,32,33);
Hbro backWheels(5,17,4,16);

void ChangeMode(int forward, int backward){
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
}

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
          PWMBoard.setPWM(15, 100, 0);
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
        PWMBoard.setPWM(15, 0, 4096);
      }
      vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    else if (currentMode == AUTO)
    {
      long range1 = UltraDist1;
      long range2 = UltraDist2;
      long range3 = ToFDist;
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Left distance %i", range1);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Right distance %i", range2);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Front distance %i", range3);

      if(range3 < minToFDist){
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Wall in front");
        
        frontWheels.stop();
        backWheels.stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);

        frontWheels.backward(255);
        backWheels.backward(255);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        frontWheels.left(255);
        backWheels.left(255);
      
        //Adjust until it turns 90 degrees
        vTaskDelay(400 / portTICK_PERIOD_MS);
        range3 = ToFDist;
        Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Front distance %i", range3);
        
        frontWheels.stop();
        backWheels.stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);

        if(range3 < minToFDist){
          frontWheels.right(255);
          backWheels.right(255);
          
          //Adjust until it turns 180 degrees
          vTaskDelay(800 / portTICK_PERIOD_MS);
        }
      }
      else if (range1 < minUltraDist)
      {
          Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close left");
          frontWheels.right(190);
          backWheels.right(190);
      }
      else if(range2 < minUltraDist)
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Too close right");
        frontWheels.left(190);
        backWheels.left(190);
      }
      else
      {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Moving forward");
        frontWheels.forward(200);
        backWheels.forward(200);
      }
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    else if (currentMode == DEMO)
    {
      vTaskDelay(1200 / portTICK_PERIOD_MS);
      if(useWheels){

        frontWheels.left(255);
        backWheels.left(255);
        vTaskDelay(1400 / portTICK_PERIOD_MS);

        frontWheels.stop();
        backWheels.stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);

        frontWheels.forward(255);
        backWheels.forward(255);
        vTaskDelay(240 / portTICK_PERIOD_MS);

        frontWheels.stop();
        backWheels.stop();
        vTaskDelay(100 / portTICK_PERIOD_MS);

        useWheels = false;
        useArm = true;
        }
    }
    else
    {
      frontWheels.stop();
      backWheels.stop();
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void UltraTask(void *parameter) {
  long qDist1, qDist2, qDist3;

  while(true){
    if(currentMode == AUTO){
      qDist1 = leftU.distance();
      UltraDist1 = qDist1;
      //xQueueSend(QUltraDist1, &qDist1, portMAX_DELAY);

      qDist2 = rightU.distance();
      UltraDist2 = qDist2;
      //xQueueSend(QUltraDist2, &qDist2, portMAX_DELAY);

      vTaskDelay(20 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

void ArmTask(void *parameter) {
  bool isTurnedOff = false;
  int BaseData, ArmBData, ArmCData, ClawData;
  int servoBasePos = 400;
  int servoArmBPos = 300;
  int servoArmCPos = 50;
  int servoClawPos = 300;

  //Startup for resetting position
  PWMBoard.setPWM(SERVO_BASE, 0, servoBasePos);
  PWMBoard.setPWM(SERVO_ARM_B, 0, servoArmBPos);
  PWMBoard.setPWM(SERVO_ARM_C, 0, servoArmCPos);
  PWMBoard.setPWM(SERVO_CLAW, 0, servoClawPos);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

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
      servoClawPos = (servoClawPos < 0) ? 0 : servoClawPos;
      servoClawPos = (servoClawPos > 670) ? 670 : servoClawPos;

      PWMBoard.setPWM(SERVO_CLAW, 0, servoClawPos);
      PWMBoard.setPWM(SERVO_ARM_C, 0, servoArmCPos);
      PWMBoard.setPWM(SERVO_ARM_B, 0, servoArmBPos);
      PWMBoard.setPWM(SERVO_BASE, 0, servoBasePos);

      vTaskDelay(6 / portTICK_PERIOD_MS);
    } 
    else if (currentMode == DEMO){
      PWMBoard.setPWM(SERVO_BASE, 0, 4096);
      PWMBoard.setPWM(SERVO_ARM_B, 0, 4096);
      PWMBoard.setPWM(SERVO_ARM_C, 0, 4096);
      PWMBoard.setPWM(SERVO_CLAW, 0, 4096);
      if(useArm){
        Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Executing demo mode: Arm");
        
        setPWMOverTime(SERVO_BASE,servoBasePos,400,1000);
        setPWMOverTime(SERVO_ARM_B,servoArmBPos,300,1000);
        setPWMOverTime(SERVO_ARM_C,servoArmCPos,50,1000);
        setPWMOverTime(SERVO_CLAW,servoClawPos,400,1000);

        setPWMOverTime(SERVO_ARM_C,servoArmCPos,200,1000);
        setPWMOverTime(SERVO_ARM_B,servoArmBPos,500,1000);

        setPWMOverTime(SERVO_ARM_C,servoArmCPos,330,1000);
        setPWMOverTime(SERVO_ARM_B,servoArmBPos,650,1000);

        setPWMOverTime(SERVO_CLAW,servoClawPos,250,1000);
        setPWMOverTime(SERVO_ARM_B,servoArmBPos,500,1000);

        setPWMOverTime(SERVO_ARM_C,servoArmCPos,200,1000);
        setPWMOverTime(SERVO_ARM_B,servoArmBPos,300,1000);

        setPWMOverTime(SERVO_ARM_C,servoArmCPos,50,1000);

        useArm = false;
      }
      vTaskDelay(250 / portTICK_PERIOD_MS);
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

void ToFTask(void *parameter){
  while (true)
  {
    if (currentMode == AUTO){
      lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

      if (measure.RangeStatus != 4) {  // phase failures have incorrect data
        ToFDist = measure.RangeMilliMeter;
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "ToF Distance (mm): %d", measure.RangeMilliMeter);
      } else {
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "ToF Sensor out of range");
      }
      vTaskDelay(20 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

//Callback function that will be executed when data is received
void DataReceivedTask(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&messageData, incomingData, sizeof(messageData));
  ChangeMode(messageData.Rbutton, messageData.Lbutton);

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

  int freq = 20000;
  int chan = 8;

  analogWriteFrequency(freq);
  analogWriteResolution(chan);
  
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing WiFi");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Logger.log(ERR_LOG, ELOG_LEVEL_ERROR, "Error initializing ESP-NOW");
    return;
  }

  Wire.begin();

  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Initializing ToF");
  lox.begin(0x29);
  if (!lox.begin()) {
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Failed to boot VL53L0X");
    while(1);
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
    1               // Core
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

  xTaskCreatePinnedToCore(
    ToFTask,        // Task function
    "ToFTask",      // Task name
    4096,           // Stack size (bytes)
    NULL,           // Parameters
    2,              // Priority
    &ToFTaskHandle, // Task handle
    0               // Core
  );
  
  Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Rover is currently in %d mode", currentMode);
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