# Arduino controlled money box (aka Salvaduino)
The aim of the project is to build a money box controlled by an Arduino board. 

The money box:
* accepts 0.50, 1 and 2 EUR coins;
* displays amount and number of coins on a 16 x 2 LCD;
* amount and number of coins are stored in EEPROM to survive power loss;
* stops accepting coins when its coin container is full;
* recognizes the owner via an RFID key fob (a Master tag is provided to change user's tag - e.g. in case of loss);
* secures the coin container via a solenoid lock;

The money box ('salvadanaio' in italian) has been nicknamed, following an established tradition, 'salvaduino'. 

### Board/Microcontroller:
Arduino UNO (Rev. 1) / Atmega328
	
### IDE:
Arduino 1.8.7

### Parts used:
* Gettoniera Programmabile (6 tipi di monete) (https://www.futurashop.it/gettoniera-programmabile-6-tipi-di-monete-6168-getton1)
* Display LCD 16x2 con interfaccia I²C (https://www.futurashop.it/display-lcd-16x2-con-interfaccia-i%C2%B2c-2846-lcd16x2ai2c)
* Modulo Read-Write per RFID - 13.56MHz (https://www.futurashop.it/modulo-read-write-rfid-8220-VMA405)
* Mini Elettroserratura 12 Vdc (https://www.futurashop.it/ELLOCK-MINI-ELETTROSERRATURA-12V)
* Misuratore distanza ultrasuoni 2-450 cm (https://www.futurashop.it/misuratore-distanza-ultrasuoni-2-450-cm-2846-misdist04)
* Convertitore di livelli logici bidirezionale (https://www.futurashop.it/convertitore-di-livelli-logici-bidirezionale-7300-llconbi)
* Adattatore Strip/Morsetto per Arduino (https://www.futurashop.it/morshield2)
* #1 pushbutton;
* #1 passive piezo buzzer;
* #2 microswitches;
* #1 5mm red LED;
* #1 180Ω 1/4W resistor;
* #1 1K0Ω 1/4W resistor;
* #1 1K5Ω 1/4W resistor;
* #1 3K3Ω 1/4W resistor;
* #1 BDX53 NPN Darlington Transistor;
* #1 1N4007 diode;
* #1 5V & 12V DC power supply;

### Libraries:

#### Purposely Developed Libraries (integral part of the project):
* Beeper_1.0
* Button_1.0
* SolenoidLock_1.0
* UltrasonicSensor_1.0
* CoinAcceptor_1.0


#### 3rd Party Libraries (used in accordance to their respective license terms):
* MFRC522 - Library for Mifare RC522 Devices (miguelbalboa) - https://github.com/miguelbalboa/rfid
* Newliquidcrystal_1.3.5 - NewLiquidCrystal Library for I2C LCD (F. Malpartida) - https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
