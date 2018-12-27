/*
  Dario B. mat. 375237
  SolenoidLock.h - Library for abstracting a solenoid lock
  (positive logic - high logic level activates solenoid).
  Released into the public domain (CC0 1.0 Universal).
*/
#ifndef SolenoidLock_h
#define SolenoidLock_h

#include "Arduino.h"

class SolenoidLock
{
  public:
    SolenoidLock(int pin);
    void release();
  private:
    int _pin;
};

#endif