#include <esp_now.h>
#include <WiFi.h>
#include <cstdint> // To define one 8 bit byte.
#include <Elog.h>

#include "math.hpp"
#include "pitches.h"

#define DEBUG_LOG 0
#define INFO_LOG 1
#define CRIT_LOG 2

int doubleClickTime = 0;
bool leftButtonBeingPressed = false;
bool rightButtonBeingPressed = false;

struct Message {
  int RX;
  int RY;
  int LX;
  int LY;
  int RButton;
  int LButton;
};

struct Joystick
{
  int XPin;
  int YPin;
  int Button;
  
  void setupPins()
  {
    analogSetPinAttenuation(XPin,ADC_11db);
    analogSetPinAttenuation(YPin,ADC_11db);
    pinMode(Button, INPUT_PULLUP);
  }

  int readX(){
    return analogRead(XPin);
  }

  int readY(){
    return analogRead(YPin);
  }

  int readButton(){
    return digitalRead(Button);
  }
};

Message message;
Message prevMessage;
Joystick RJoystick;
Joystick LJoystick;

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t myBoardAddress[] = {0x00, 0x4b, 0x12, 0x3a, 0x2d, 0xec};

esp_now_peer_info_t peerInfo;

int SwitchButtonState(bool &buttonBeingPressed, int state){//, bool buttonBeingPressed){
  if (!buttonBeingPressed && state == 0) //Button is just pressed
  {
    buttonBeingPressed = true;
    return 1;
  } 
  else if (state == 1) //Button is released
  {
    buttonBeingPressed = false;
  }
  return 0;
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "To receiver MAC address: %d", macStr);
  
  //TODO replace with Logger
  /*Serial.print("To receiver MAC address: ");
  Serial.println(macStr);
  Serial.print("Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ?
  "Receiver acknowledged reception" : 
  "Receiver did not acknowledge reception");
  /*
  Serial.print("\r\n");
  */
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // We want messages with DEBUG level and lower
  Logger.registerSerial(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Debug"); 
  Logger.registerSerial(INFO_LOG, ELOG_LEVEL_INFO, "Info"); 
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Logger.log(CRIT_LOG, ELOG_LEVEL_CRITICAL, "Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, myBoardAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Logger.log(CRIT_LOG, ELOG_LEVEL_CRITICAL, "Failed to add peer");
    return;
  }
  
  RJoystick.XPin = 33;
  RJoystick.YPin = 32;
  RJoystick.Button = 0;

  LJoystick.XPin = 34;
  LJoystick.YPin = 35;
  LJoystick.Button = 21;

  RJoystick.setupPins();
  LJoystick.setupPins();
}

void loop() {
  int RState = RJoystick.readButton();
  message.RX = RJoystick.readX();
  message.RY = RJoystick.readY();
  message.RButton = SwitchButtonState(rightButtonBeingPressed,RState);//, RJoystick.isBeingPressed);

  int LState = LJoystick.readButton();
  message.LX = LJoystick.readX();
  message.LY = LJoystick.readY();
  message.LButton = SwitchButtonState(leftButtonBeingPressed,LState);//, LJoystick.isBeingPressed);

  //Handles double click, due to loop being 1 millisecond, the timing will be 100 milliseconds
  /*if(message.RButton && doubleClickTime > 0){
    message.RButton = 2;
    doubleClickTime = 0;
  } else if (message.RButton && doubleClickTime <= 0){
    doubleClickTime = 100;
  } else if (doubleClickTime > 0) {
    //Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "DoubleClickTime = %d", doubleClickTime);
    doubleClickTime--;
  }*/

  if (!isClose(message.RX, prevMessage.RX, 100) || 
      !isClose(message.RY, prevMessage.RY, 100) ||
      !isClose(message.LX, prevMessage.LX, 100) ||
      !isClose(message.LY, prevMessage.LY, 100) ||
      message.RButton ||
      message.LButton)
  {
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(myBoardAddress, (uint8_t *) &message, sizeof(message));
    if (result == ESP_OK) {
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "---------------------------------------------------------------");
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Sent with success. Voltage value: ");
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Rx: %d", message.RX);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Ry: %d", message.RY);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Lx: %d", message.LX);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Ly: %d", message.LY);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Button: %i", message.RButton);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Button: %i", message.LButton);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "---------------------------------------------------------------");

      prevMessage.RX = message.RX;
      prevMessage.RY = message.RY;
      prevMessage.LX = message.LX;
      prevMessage.LY = message.LY;
      //prevMessage.RButton = message.RButton;
      //prevMessage.LButton = message.LButton;
    }
    else {
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Error sending the data");
    }
  }
  delay(1);
}