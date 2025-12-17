#include <Arduino.h>
#include <Elog.h>

#define DEBUG_LOG 0
#define INFO_LOG 1
#define ERR_LOG 2

struct UltraSound
{
    int trigPin;
    int echoPin;

    long cm, duration;

    UltraSound(int trigPinVal, int echoPinVal){
        trigPin = trigPinVal;
        echoPin = echoPinVal;
    }

    void setupPins() 
    {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    long distance(){
        digitalWrite(trigPin, LOW);
        delayMicroseconds(5);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        duration = pulseIn(echoPin, HIGH);
        
        // Convert the time into a distance
        cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
        
        Logger.log(DEBUG_LOG, ELOG_LEVEL_DEBUG, "Distance Measurement: %d", cm);
        //Serial.println(cm);
        return cm;
    }
};