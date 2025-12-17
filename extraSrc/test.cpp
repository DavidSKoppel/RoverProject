#include "Arm.hpp"

void setup() {
  Serial.begin(115200);
  // Open serial communications and wait for port to open:
  Serial.print("beginning");
  while(!PWMBoard.begin()){
    delay(1);
    Serial.print(".");
  }
  PWMBoard.setPWMFreq(50);
}

void loop() {
  /*
    setPWMOverTime(servoBase, 90,180,1000);
    setPWMOverTime(servoBase, 180,0,1000);
    setPWMOverTime(servoBase, 0,90,1000);

    setPWMOverTime(servoArmR, 0,90,1000);

    setPWMOverTime(servoClaw, 20,90,1000);
    setPWMOverTime(servoClaw, 90,20,1000);

    setPWMOverTime(servoArmR, 90,0,1000);
    */
    delay(1000);
}