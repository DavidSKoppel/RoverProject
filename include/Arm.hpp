#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVOMIN 150
#define SERVOMAX 670

enum Servo {
    SERVO_BASE = 0,
    SERVO_ARM_B = 1,
    SERVO_ARM_C = 2,
    SERVO_CLAW = 3,
};

Adafruit_PWMServoDriver PWMBoard = Adafruit_PWMServoDriver();

int servoConvertDegrees(int degrees) 
{
    int position = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
    return position;
}

void setServoPos(Servo servo, int speed, int currentPos){
    int newPos = speed * 0.00053;
    currentPos += newPos;
    currentPos = (currentPos < 100) ? 100 : currentPos;
    currentPos = (currentPos > 670) ? 670 : currentPos;

    PWMBoard.setPWM(servo, 0, currentPos);
}

void setPWMOverTime(Servo servo, int& fromOut, int to, int milliTime){
    if(fromOut != to){
        int steps = abs(to - fromOut);          

        int delayTime = milliTime / steps;

        int step = (to > fromOut) ? 1 : -1;

        for (int pos = fromOut; pos != to; pos += step) {
            //int posDegrees = servoConvertDegrees(posDegrees);
            PWMBoard.setPWM(servo, 0, pos);
            vTaskDelay(delayTime / portTICK_PERIOD_MS);
            //delay(delayTime);
        }
    }
    fromOut = to;
}