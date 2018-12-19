/*
  Dario B. mat. 375237
  
  Button.cpp - Library for abstracting a push button
  (pullup - when pressed reads LOW).
  Released into the public domain (CC0 1.0 Universal).
*/

#include "Arduino.h"
#include "Button.h"

Button::Button(int pin){
	pinMode(pin, INPUT_PULLUP);
	_pin = pin;
	_previousReading = false;
	_isBtnPressed = false;
}

bool Button::isPressed(){
	_thisTick = millis();
	_currentReading = LOW == digitalRead(_pin);
	if (_currentReading != _previousReading) {
		//switch changed, reset the debouncing timer
		_previousReading = _currentReading;
		_lastDebounceTime = _thisTick;
	}		
	if((_currentReading != _isBtnPressed) && ((_thisTick - _lastDebounceTime) > DEBOUNCE_INTERVAL)){
		_isBtnPressed = _currentReading;
	}
	return _isBtnPressed;
}
