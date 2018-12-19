/*
  Beeper.h - Library for abstracting a beeper
  Released into the public domain (CC0 1.0 Universal).
*/
#ifndef Beeper_h
#define Beeper_h

#include "Arduino.h"

class Beeper
{
  public:
    Beeper(int pin);
    void beep(bool);
  private:
    int _pin;
};

#endif