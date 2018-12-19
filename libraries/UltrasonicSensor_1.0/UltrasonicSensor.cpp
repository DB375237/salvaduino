/*
  Dario B. mat. 375237
  
  UltrasonicSensor.h - Library for abstracting Ultrasonic Sensor HC-SR04 
  based on https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
  Released into the public domain (CC0 1.0 Universal).
*/

#include "Arduino.h"
#include "UltrasonicSensor.h"

#define DELAY_US 10

UltrasonicSensor::UltrasonicSensor(int trigPin, int echoPin){
	pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
	pinMode(echoPin, INPUT);
	_trigPin = trigPin;
	_echoPin = echoPin;
}

unsigned int UltrasonicSensor::measure(){
	long duration;
	
	// Clears the trigPin
	digitalWrite(_trigPin, LOW);
	delayMicroseconds(DELAY_US);
	
	// Sets the trigPin on HIGH state for 10 micro seconds
	digitalWrite(_trigPin, HIGH);
	delayMicroseconds(DELAY_US);
	digitalWrite(_trigPin, LOW);
	
	// Reads the echoPin, returns the sound wave travel time in microseconds
	duration = pulseIn(_echoPin, HIGH);
	
	// Calculating the distance in mm
	return duration * 0.34 / 2;
}
