/*
 * Dario B. mat. 375237
 * released into public domain (CC0 1.0 Universal)
 *
 * salvaduino.h
 */

#ifdef DEBUG
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTHEX(x) Serial.print(x, HEX);
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTHEX(x)
  #define DEBUG_PRINTLN(x)
#endif

#define EEPROM_AMOUNT_DELIM '$'
#define EEPROM_USER_SECURITYCODE_DELIM '?'
#define EEPROM_MASTER_SECURITYCODE_DELIM '*'
#define EEPROM_END_DELIM '#'
#define SECURITY_CODE_LEN 4
#define EEPROM_COINS_DELIM '@'


#define EEPROM_ADDRESS_START 0
#define EEPROM_ADDRESS_AMOUNT_DELIM EEPROM_ADDRESS_START
#define EEPROM_ADDRESS_AMOUNT (EEPROM_ADDRESS_AMOUNT_DELIM + sizeof(char))

#define EEPROM_ADDRESS_USERSECURITYCODE_DELIM (EEPROM_ADDRESS_AMOUNT + sizeof(unsigned int)) 
#define EEPROM_ADDRESS_USERSECURITYCODE (EEPROM_ADDRESS_USERSECURITYCODE_DELIM + sizeof(char)) 

#define EEPROM_ADDRESS_MASTERSECURITYCODE_DELIM (EEPROM_ADDRESS_USERSECURITYCODE + SECURITY_CODE_LEN) 
#define EEPROM_ADDRESS_MASTERSECURITYCODE (EEPROM_ADDRESS_MASTERSECURITYCODE_DELIM + sizeof(char)) 

#define EEPROM_ADDRESS_COINS_DELIM (EEPROM_ADDRESS_MASTERSECURITYCODE + SECURITY_CODE_LEN)
#define EEPROM_ADDRESS_COIN0 (EEPROM_ADDRESS_COINS_DELIM + sizeof(char))
#define EEPROM_ADDRESS_COIN1 (EEPROM_ADDRESS_COIN0 + sizeof(unsigned int))
#define EEPROM_ADDRESS_COIN2 (EEPROM_ADDRESS_COIN1 + sizeof(unsigned int))

#define EEPROM_ADDRESS_END (EEPROM_ADDRESS_COIN2 + sizeof(unsigned int))

//#define COIN_ACCEPTOR_SERIAL_SPEED 4800
#define COIN_50CT 5
#define COIN_1EUR 10
#define COIN_2EUR 20
#define NUM_COINS 3

#define FALSE 0
#define TRUE (~FALSE)
#define SUCCESS (true)
#define FAIL (false)

#define MASTER_TAG true
#define USER_TAG  false

#define I2C_LCD_ADDR 0x27 //LCD I2C address 
#define LCD_EN 2
#define LCD_RW 1
#define LCD_RS 0
#define LCD_BL 3
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7
#define LCD_ROW_LEN 16

#define DISPLAY_COIN_TIMEOUT 1000
#define DISPLAY_STATUS_TIMEOUT 1500
#define BLINK_INTERVAL 1000UL //must be 1s. for proper timeout

#define SS_PIN 10 //Slave select pin for SPI
#define RST_PIN 9 //reset pin of RFID
#define PIN_D3 3
#define PIN_D4 4
#define PIN_D5 5
#define PIN_D7 7
#define PIN_D8 8

#define _10cm5  105
#define _10cm0  100
#define _5cm0   50
#define MAX_READINGS 8

#define PICC_UID_SIZE 4
#define TAG_SCAN_TIMEOUT 10

enum TAG_READ_TYPE {NO_TAG_READ, UNKNOWN_TAG_READ, USER_TAG_READ, MASTER_TAG_READ};
enum PiggyBankStatusType {ACCEPTING_COINS, TRAY_ERROR, DOOR_OPEN, RESET_AMOUNT, INITIALIZING = -1, WAITING_NEW_MASTER_TAG = -2, WAITING_NEW_USER_TAG = -3, UNDEFINED = -99};
typedef struct
{
  unsigned int NumberOfCoins[NUM_COINS];
	unsigned int Amount; //amount is stored in cents, 1 tenth of nominal value. e.g. 20 for 2.00 eur
	PiggyBankStatusType CurrentStatus;
  PiggyBankStatusType PreviousStatus;
  bool isTrayMisplaced;
  bool isDoorOpen;
  bool isResetAmountRequested;
  bool MasterSecurityCodeDefined;
  bool UserSecurityCodeDefined;
	byte UserSecurityCode[SECURITY_CODE_LEN]; 
  byte MasterSecurityCode[SECURITY_CODE_LEN];
}PiggyBankType;
