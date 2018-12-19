/*
  Dario B. mat. 375237
  
  Beeper.cpp - Library for abstracting a beeper
  Released into the public domain (CC0 1.0 Universal).
*/

#include "Arduino.h"
#include "Beeper.h"

#define NOTE_CS8 4435
#define DURATION 150

Beeper::Beeper(int pin)
{
  _pin = pin;
}

void Beeper::beep(bool success)
{
  tone(_pin, NOTE_CS8, DURATION);
  if(!success){
	delay(DURATION<<1);
	tone(_pin, NOTE_CS8, DURATION);
  }
}
