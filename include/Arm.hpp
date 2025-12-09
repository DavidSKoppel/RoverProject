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

void setPWMOverTime(int servo, int from, int to, int milliTime){
    int steps = abs(to - from);          

    int delayTime = milliTime / steps;

    int step = (to > from) ? 1 : -1;

    for (int posDegrees = from; posDegrees != to; posDegrees += step) {
        int pos = convertDegrees(posDegrees);
        PWMBoard.setPWM(servo, 0, pos);
        vTaskDelay(delayTime / portTICK_PERIOD_MS);
        //delay(delayTime);
    }
}