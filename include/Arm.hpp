#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVOMIN 150
#define SERVOMAX 600

enum Servo {
    SERVO_BASE = 0,
    SERVO_ARM_R = 1,
    SERVO_ARM_L = 2,
    SERVO_ARM_CLAW = 3,
};

Adafruit_PWMServoDriver PWMBoard = Adafruit_PWMServoDriver();

int convertDegrees(int degrees) 
{
    int position = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
    return position;
}

void setPWMOverTime(Servo servo, int from, int to, int milliTime){
    int steps = abs(to - from);          

    int delayTime = milliTime / steps;

    int step = (to > from) ? 1 : -1;

    for (int posDegrees = from; posDegrees != to; posDegrees += step) {
        int pos = convertDegrees(posDegrees);
        PWMBoard.setPWM(servo, 0, pos);
        //vTaskDelay(delayTime / portTICK_PERIOD_MS);
        delay(delayTime);
    }
}