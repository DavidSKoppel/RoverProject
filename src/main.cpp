#include <Arduino.h> // VSCode klager tag jer ikke af det

// ESPIDF kræver extern C. Main skal være .cpp
// Ingen setup + loop; alt i app_main i stedet
// Sæt CONFIG_FREERTOS_HZ=1000 i sdkconfig.esp32dev
// I platformio.ini sæt framework = arduino, espidf

extern "C" void app_main(void) {
  
  const int pin = 1;

  //Void Setup(){
  Serial.begin(9600);
  //}
  
  //Void Loop()
  while (true)
  {

  }
  //}
}