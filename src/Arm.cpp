#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVOMIN 150
#define SERVOMAX 600

const int servoBase = 0;
const int servoArmR = 1;
const int servoArmL = 2;
const int servoClaw = 3;

Adafruit_PWMServoDriver PWMBoard = Adafruit_PWMServoDriver();

int convertDegrees(int degrees) 
{
    int position = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
    return position;
}

void servoMoveSpeed(int servo, int from, int to, int milliTime){
    int steps = abs(to - from);          

    int delayTime = milliTime / steps;

    int step = (to > from) ? 1 : -1;

    for (int posDegrees = from; posDegrees != to; posDegrees += step) {
        int pos = convertDegrees(posDegrees);
        PWMBoard.setPWM(servo, 0, pos);
        delay(delayTime);
    }
}

void setup() {
    Serial.begin(115200);
    while(!PWMBoard.begin()){
        delay(1);
    }
    PWMBoard.setPWMFreq(50);
    delay(30);
}

void loop() {
    servoMoveSpeed(servoBase, 90,180,1000);
    servoMoveSpeed(servoBase, 180,0,1000);
    servoMoveSpeed(servoBase, 0,90,1000);

    servoMoveSpeed(servoArmR, 0,40,1000);

    servoMoveSpeed(servoClaw, 0,90,1000);
    servoMoveSpeed(servoClaw, 90,0,1000);

    servoMoveSpeed(servoArmR, 40,0,1000);
}

