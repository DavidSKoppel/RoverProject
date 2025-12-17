#include <esp_now.h>
#include <WiFi.h>
#include <cstdint> // To define one 8 bit byte.
#include <Elog.h>

#include "math.hpp"

#define DEBUG_LOG 0
#define INFO_LOG 1
#define CRIT_LOG 2

int doubleClickTime = 0;
bool buttonBeingPressed = false;

struct Message {
  int x;
  int y;
  int state;
};

struct Joystick
{
  int XPin = 33;
  int YPin = 32;
  int SwitchButton = 0;

  void setupPins() 
  {
    analogSetPinAttenuation(XPin,ADC_11db);
    analogSetPinAttenuation(YPin,ADC_11db);
    pinMode(SwitchButton, INPUT_PULLUP);
  }

  int readX(){
    return analogRead(XPin);
  }

  int readY(){
    return analogRead(YPin);
  }

  int readButton(){
    return digitalRead(SwitchButton);
  }
};

Message message;
Message prevMessage;
Joystick joystick;

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t myBoardAddress[] = {0x00, 0x4b, 0x12, 0x3b, 0x47, 0x00};

esp_now_peer_info_t peerInfo;

int SwitchButtonState(int state){
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
  Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "To receiver MAC address: %d", macStr);
  
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

  joystick.setupPins();
}

void loop() {
  int state = joystick.readButton();
  message.x = joystick.readX();
  message.y = joystick.readY();
  message.state = SwitchButtonState(state);

  //Handles double click
  if(message.state && doubleClickTime > 0){
    message.state = 2;
    doubleClickTime = 0;
  } else if (message.state && doubleClickTime <= 0){
    doubleClickTime = 100;
  } else if (doubleClickTime > 0) {
    Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "DoubleClickTime = %d", doubleClickTime);
    doubleClickTime--;
  }

  if (!isClose(message.x, prevMessage.x, 100) || 
      !isClose(message.y, prevMessage.y, 100) ||
      message.state)
  {
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(myBoardAddress, (uint8_t *) &message, sizeof(message));
    if (result == ESP_OK) {
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "---------------------------------------------------------------");
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Sent with success. Voltage value: ");
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "x: %d", message.x);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "y: %d", message.y);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "state: %i", message.state);
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "---------------------------------------------------------------");

      prevMessage.x = message.x;
      prevMessage.y = message.y;
      prevMessage.state = message.state;
    }
    else {
      Logger.log(INFO_LOG, ELOG_LEVEL_INFO, "Error sending the data");
    }
  }
  delay(1);
}