#include <Arduino.h>

struct Hbro
{
  int motorA1;
  int motorA2;
  int motorB1;
  int motorB2;

  Hbro(int motorA1Val, int motorA2Val, int motorB1Val, int motorB2Val) {
        motorA1 = motorA1Val;
        motorA2 = motorA2Val;
        motorB1 = motorB1Val;
        motorB2 = motorB2Val;
    }

    void setupPins() 
    {
        pinMode(motorA1, OUTPUT);
        pinMode(motorA2, OUTPUT);
        pinMode(motorB1, OUTPUT);
        pinMode(motorB2, OUTPUT);
    }
    void forward(int speed)
    {
        analogWrite(motorA1, speed);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, speed);
        analogWrite(motorB2, 0);
    }
    void backward(int speed)
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, speed);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, speed);
    }
    void right(int speed)
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, speed);
        analogWrite(motorB1, speed);
        analogWrite(motorB2, 0);
    }
    void left(int speed)
    {
        analogWrite(motorA1, speed);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, speed);
    }
    void stop()
    {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
    }
};

