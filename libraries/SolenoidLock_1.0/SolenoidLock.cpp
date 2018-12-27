/*
  Dario B. mat. 375237
  SolenoidLock.cpp - Library for abstracting a solenoid lock
  (positive logic - high logic level activates solenoid).
  Released into the public domain (CC0 1.0 Universal).
*/

#include "Arduino.h"
#include "SolenoidLock.h"

#define RELEASE_DURATION 500

SolenoidLock::SolenoidLock(int pin)
{
	pinMode(pin, OUTPUT);
  _pin = pin;
}

void SolenoidLock::release()
{
  digitalWrite(_pin, HIGH);
  delay(RELEASE_DURATION);
  digitalWrite(_pin, LOW);
}
