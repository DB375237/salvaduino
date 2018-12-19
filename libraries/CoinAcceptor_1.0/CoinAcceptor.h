/*
  Dario B. mat. 375237
  
  CoinAcceptor.cpp - Library for abstracting Sparkfun DG600F Coin Acceptor - Programmable (6 coin types)
  Released into the public domain (CC0 1.0 Universal).
  
  Dip Switch Functions Setting:
  SW3 Transmitting	: ON (RS232)
  SW4 Inhibiting	: ON (High Level forbid accepting coins)
*/
#ifndef CoinAcceptor_h
#define CoinAcceptor_h

#include "Arduino.h"

class CoinAcceptor{
  public:
    CoinAcceptor(int pin);
    void enableCoins(bool enable);
	bool coinsAvalilable(void);
	int readCoin(void);
	void initSerialInterface(void);
	bool isEnabled;	
  private:
    int _pin;
};

#endif