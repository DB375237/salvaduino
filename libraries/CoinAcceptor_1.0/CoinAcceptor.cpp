/*
  Dario B. mat. 375237
  
  CoinAcceptor.cpp - Library for abstracting Sparkfun DG600F Coin Acceptor - Programmable (6 coin types)
  Released into the public domain (CC0 1.0 Universal).
  
  Dip Switch Functions Setting:
  SW3 Transmitting	: ON (RS232)
  SW4 Inhibiting	: ON (High Level or no signal forbid accepting coins, Low Level accepts coins)
*/

#include "Arduino.h"
#include "CoinAcceptor.h"

#define COIN_ACCEPTOR_SERIAL_SPEED 4800

#define COIN_50CT 1
#define COIN_1EUR 2
#define COIN_2EUR 4
#define COIN_BASE_AMOUNT 5 //50 cent is display xxx.5

CoinAcceptor::CoinAcceptor(int pin){
	pinMode(pin, OUTPUT);
	_pin = pin;
	isEnabled = false;
	enableCoins(isEnabled);
}

void CoinAcceptor::initSerialInterface(void){
	//DG600F Coin Acceptor: according to datasheet UART protocol is Start, 8 data bits, even parity, stop bit 
	Serial.begin(COIN_ACCEPTOR_SERIAL_SPEED, SERIAL_8E1);
}

void CoinAcceptor::enableCoins(bool enable){
	isEnabled = enable;
	digitalWrite(_pin, enable ? LOW : HIGH);
}

bool CoinAcceptor::coinsAvalilable(void){
	return (Serial.available() > 0);
}

int CoinAcceptor::readCoin(void){
	int incomingByte = Serial.read();
	switch (incomingByte){
	  case COIN_50CT:     
	  case COIN_1EUR:
	  case COIN_2EUR:
		return incomingByte * COIN_BASE_AMOUNT;
		
	  default:
		//this is not supposed to happen but some users reported framing errors
		//in serial data from coin acceptor. This may be related to wrong UART setting.
		//see SPARKFUN documentation/demo projects
		return 0; 
	}
}