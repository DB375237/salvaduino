/*
  Dario B. mat. 375237
  
  Button.h - Library for abstracting a push button
  (pullup - when pressed reads LOW).
  Released into the public domain (CC0 1.0 Universal).
*/
#ifndef Button_h
#define Button_h

#include "Arduino.h"
#define DEBOUNCE_INTERVAL 20

class Button{
  public:
    Button(int pin);
    bool isPressed();
  private:
    int _pin;
	bool _currentReading, _previousReading, _isBtnPressed;
	unsigned long _lastDebounceTime, _thisTick;
};

#endif