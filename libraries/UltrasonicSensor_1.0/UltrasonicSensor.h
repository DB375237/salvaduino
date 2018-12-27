/*
  Dario B. mat. 375237
  
  UltrasonicSensor.h - Library for abstracting Ultrasonic Sensor HC-SR04 
  based on https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
  Released into the public domain (CC0 1.0 Universal).
*/
#ifndef UltrasonicSensor_h
#define UltrasonicSensor_h

#include "Arduino.h"

class UltrasonicSensor{
  public:
    UltrasonicSensor(int trigPin, int echoPin);
    unsigned int measure();
  private:
    int _trigPin, _echoPin;
};

#endif