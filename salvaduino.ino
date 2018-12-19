/*
 * D. Bellisario
 * released into public domain (Creative Commons 0)
 *
 * salvaduino.ino
 */
 
#define DEBUG 

/************************* INCLUDE **************************************/
#include <Wire.h>                 //Wire Library for I2C
#include <SPI.h>                  //SPI for RFID
#include <EEPROM.h>               //EEPROM for saving config/amount
#include <LiquidCrystal_I2C.h>    //NewLiquidCrystal Library for I2C LCD (F. Malpartida) - https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
#include <MFRC522.h>              //Library for Mifare RC522 Devices (miguelbalboa) - https://github.com/miguelbalboa/rfid
//
#include <SolenoidLock.h>
#include <Button.h>
#include <Beeper.h>
#include <CoinAcceptor.h>
#include <UltrasonicSensor.h>
//
#include "salvaduino.h"

/*************************************************************** 
 *  Global variables
 ***************************************************************/
static char buffer[LCD_ROW_LEN + 1];
static PiggyBankType piggyBank;

static MFRC522 mfrc522(SS_PIN, RST_PIN);    //MFRC522 RFID Reader
static LiquidCrystal_I2C lcd(I2C_LCD_ADDR, LCD_EN, LCD_RW, LCD_RS, LCD_D4, LCD_D5, LCD_D6, LCD_D7, LCD_BL, POSITIVE);
static UltrasonicSensor ultrasonic_sensor(PIN_D7, PIN_D8);
static SolenoidLock lock(PIN_D5);
static CoinAcceptor coinAcceptor(PIN_D4);
static Button tray_btn(A0);                 //tray misplaced/missing
static Button reset_amount_btn(A1);         //reset amount
static Button door_btn(A2);                 //door open
static Beeper beeper(PIN_D3);               //door open

static byte readCard[PICC_UID_SIZE];        //Stores scanned ID read from RFID Module
static byte tmp_readCard[PICC_UID_SIZE];    //Stores scanned tmp ID read when associating/changing tags
static bool previuosBtnStatus, heartbeat = true; 
static unsigned long thisTick, lastTick = millis();

static byte rfidReadCount, coinIdx = 0;//, heartbeat2 = 0;
static int tagScanTimer = -1;
  

/***************************************************************
 * setup()
 * setup code here, to run once
 ***************************************************************/
void setup() {
  // Set display type as 16 char, 2 rows
  lcd.begin(16,2); 
  //
  SPI.begin();                                              // MFRC522 Hardware uses SPI protocol
  //
  coinAcceptor.initSerialInterface();
  coinAcceptor.isEnabled = false;

  mfrc522.PCD_Init(); // Initialize MFRC522 Hardware
  #ifdef DEBUG
  CheckRFIDReader();  // Show details of PCD - MFRC522 Card Reader details
  #endif  
  //
  piggyBank.PreviousStatus  = UNDEFINED;
  piggyBank.CurrentStatus   = UNDEFINED;
  
  piggyBank.isResetAmountRequested  = reset_amount_btn.isPressed();
  piggyBank.isTrayMisplaced         = tray_btn.isPressed();
  piggyBank.isDoorOpen              = door_btn.isPressed();
  
  piggyBank.NumberOfCoins[0]        = 0;
  piggyBank.NumberOfCoins[1]        = 0;
  piggyBank.NumberOfCoins[2]        = 0;
  
  piggyBank.Amount = 0;
  
  piggyBank.MasterSecurityCodeDefined = false;
  piggyBank.UserSecurityCodeDefined   = false;
  
  piggyBank.UserSecurityCode[0] = '0';
  piggyBank.UserSecurityCode[1] = '0';
  piggyBank.UserSecurityCode[2] = '0';
  piggyBank.UserSecurityCode[3] = '0';
  
  piggyBank.MasterSecurityCode[0] = '0';
  piggyBank.MasterSecurityCode[1] = '0';
  piggyBank.MasterSecurityCode[2] = '0';
  piggyBank.MasterSecurityCode[3] = '0';
  //
  initFromEEPROM(&piggyBank);
  //
  LCD_Print(F("-- Salvaduino --"), F("--   ver.1.0  --"), DISPLAY_STATUS_TIMEOUT);
  //
  setStatus(&piggyBank, INITIALIZING);
  DEBUG_PRINTLN(F("-- end of setup() --"));  
}

/***************************************************************
 * loop()
 * your main code here, to run repeatedly
 ***************************************************************/
void loop() {
  thisTick = millis();
  
  if(thisTick - lastTick >= BLINK_INTERVAL){
    lastTick = millis(); // reset last time fired
    heartbeat = !heartbeat;
    if(tagScanTimer) tagScanTimer--;    

    
    //heartbeat2 = ++heartbeat2 % (NUM_COINS * COIN_50CT);
    //coinIdx = heartbeat2 / COIN_50CT;
    coinIdx = ++coinIdx % NUM_COINS;
    //DEBUG_PRINTLN(coinIdx);
  }

  //read buttons - debounced
  previuosBtnStatus = piggyBank.isTrayMisplaced; 
  piggyBank.isTrayMisplaced = tray_btn.isPressed();
  
  #ifdef DEBUG  
  if(previuosBtnStatus != piggyBank.isTrayMisplaced){
    DEBUG_PRINT(F("tray_button "));
    DEBUG_PRINTLN(piggyBank.isTrayMisplaced ? "open" : "close");    
  }
  #endif
    
  previuosBtnStatus = piggyBank.isDoorOpen;
  piggyBank.isDoorOpen = door_btn.isPressed();
  
  #ifdef DEBUG
  if(previuosBtnStatus != piggyBank.isDoorOpen){
    DEBUG_PRINT(F("door_btn "));
    DEBUG_PRINTLN(piggyBank.isDoorOpen ? "open" : "close");    
  }
  #endif

  previuosBtnStatus = piggyBank.isResetAmountRequested;
  piggyBank.isResetAmountRequested = reset_amount_btn.isPressed();

  #ifdef DEBUG
  if(previuosBtnStatus != piggyBank.isResetAmountRequested){
    DEBUG_PRINT(F("reset_amount_btn "));
    DEBUG_PRINTLN(piggyBank.isResetAmountRequested ? "pressed" : "released");    
  }
  #endif  
  
  switch (piggyBank.CurrentStatus) {
    case INITIALIZING:
      //on first use, master tag hasn't been defined yet, so read master tag and store it.
      //on first use, user tag hasn't been defined yet. It can be defined with standard user code recovery procedure 
      if(!piggyBank.MasterSecurityCodeDefined){
        rfidReadCount = 0;
        setStatus(&piggyBank, WAITING_NEW_MASTER_TAG);
        break;
      }    
      if(piggyBank.isTrayMisplaced || !isSpaceAvailable()){
        setStatus(&piggyBank, TRAY_ERROR);
        break;
      }
      if(piggyBank.isDoorOpen){
        setStatus(&piggyBank, DOOR_OPEN);
        break;
      }
      coinAcceptor.isEnabled = true;
      beeper.beep(SUCCESS);
      setStatus(&piggyBank, ACCEPTING_COINS);
      break;
      
    case WAITING_NEW_USER_TAG:
      displayUserTagAlert();
      if(!tagScanTimer){
        DEBUG_PRINTLN(F("User Code - timed out"));
        beeper.beep(FAIL);
        setStatus(&piggyBank, piggyBank.PreviousStatus);
        break;
      }           
      if(readRFID()){
        DEBUG_PRINTLN(rfidReadCount ? F("User Code - second scan") : F("User Code - first scan"));        
        if(!rfidReadCount){
          if(checkMasterTag(&piggyBank)){
            DEBUG_PRINTLN("User Code - first scan error: User Code == Master Code");
            rfidReadCount = 0;
            beeper.beep(FAIL);
            LCD_Print(F("-- Tag UTENTE --"), F("-- lettura KO --"), DISPLAY_STATUS_TIMEOUT);                                
          }else{
            DEBUG_PRINTLN("User Code - first scan successful");            
            tmp_readCard[0] = readCard[0];
            tmp_readCard[1] = readCard[1];
            tmp_readCard[2] = readCard[2];
            tmp_readCard[3] = readCard[3];            
            rfidReadCount++;
            beeper.beep(SUCCESS);
            LCD_Print(F("-- Tag UTENTE --"), F("-- lettura OK --"), DISPLAY_STATUS_TIMEOUT);   
          }
          tagScanTimer = TAG_SCAN_TIMEOUT;
        }else{
          if(checkTmpTag()){
            DEBUG_PRINTLN("User Code - second scan successful");
            piggyBank.UserSecurityCode[0] = readCard[0];
            piggyBank.UserSecurityCode[1] = readCard[1];
            piggyBank.UserSecurityCode[2] = readCard[2];
            piggyBank.UserSecurityCode[3] = readCard[3];            
            piggyBank.UserSecurityCodeDefined = true;
            saveUserCodeToEEPROM(&piggyBank);
            beeper.beep(SUCCESS);
            LCD_Print(F("-- Tag UTENTE --"), F("--- salvato! ---"), DISPLAY_STATUS_TIMEOUT);
            setStatus(&piggyBank, piggyBank.PreviousStatus);         
          }else{
            DEBUG_PRINTLN("User Code - second scan mismatch");
            beeper.beep(FAIL);
            LCD_Print(F("-- Tag UTENTE --"), F("-- lettura KO --"), DISPLAY_STATUS_TIMEOUT);     
            rfidReadCount = 0;
            tagScanTimer = TAG_SCAN_TIMEOUT;                               
          }          
        }
      }      
      break;
      
    case WAITING_NEW_MASTER_TAG:
      displayMasterCodeAlert(); 
      if(readRFID()){
        if(!rfidReadCount){
          DEBUG_PRINTLN("Master Code - first scan");
          tmp_readCard[0] = readCard[0];
          tmp_readCard[1] = readCard[1];
          tmp_readCard[2] = readCard[2];
          tmp_readCard[3] = readCard[3];      
          rfidReadCount++;
          beeper.beep(SUCCESS);
          LCD_Print(F("-- Tag MASTER --"), F("-- lettura OK --"), DISPLAY_STATUS_TIMEOUT);       
        }else{
          if(checkTmpTag()){
            DEBUG_PRINTLN("Master Code - second scan successful");
            piggyBank.MasterSecurityCode[0] = readCard[0];
            piggyBank.MasterSecurityCode[1] = readCard[1];
            piggyBank.MasterSecurityCode[2] = readCard[2];
            piggyBank.MasterSecurityCode[3] = readCard[3];            
            piggyBank.MasterSecurityCodeDefined = true;
            saveMasterCodeToEEPROM(&piggyBank);
            beeper.beep(SUCCESS);
            LCD_Print(F("-- Tag MASTER --"), F("--- salvato! ---"), DISPLAY_STATUS_TIMEOUT);             
            setStatus(&piggyBank, INITIALIZING);         
          }else{
            DEBUG_PRINTLN("Master Code - second scan mismatch");
            rfidReadCount = 0;
            beeper.beep(FAIL);
            LCD_Print(F("-- Tag MASTER --"), F("-- lettura KO --"), DISPLAY_STATUS_TIMEOUT);                      
          }          
        }
      }
      break;
      
    case ACCEPTING_COINS:
      //check conditions that may prevent coins from being accepted
      if(piggyBank.isTrayMisplaced || !isSpaceAvailable()){
        setStatus(&piggyBank, TRAY_ERROR);
        break;
      }
      if(piggyBank.isDoorOpen){
        setStatus(&piggyBank, DOOR_OPEN);
        break;
      }
      displayNumberOfCoins(&piggyBank);      
      acceptCoins(&piggyBank);    
      //check remaining condition: tag is swiped
      switch(checkTag(&piggyBank)){
        case NO_TAG_READ:
          break;
          
        case UNKNOWN_TAG_READ:
          beeper.beep(FAIL);
          displayUnknownTagAlert();
          //reset display
          displayAmount(&piggyBank);
          displayNumberOfCoins(&piggyBank);         
          break;        
          
        case USER_TAG_READ:
          beeper.beep(SUCCESS);
          setStatus(&piggyBank, DOOR_OPEN); 
          displayKnownTagMessage(USER_TAG);        
          break;  
                
        case MASTER_TAG_READ:
          //user tag recovery proc.
          beeper.beep(SUCCESS);
          setStatus(&piggyBank, WAITING_NEW_USER_TAG);  
          displayKnownTagMessage(MASTER_TAG);      
          break;
      }
      break;
      
    case TRAY_ERROR:
      displayTrayErrorAlert();     
      //we accept that tray error may 'fix' itself without opening the piggybank
      //(e.g. a whack on the side!)
      if(!piggyBank.isTrayMisplaced && isSpaceAvailable()){
        setStatus(&piggyBank, piggyBank.PreviousStatus);
        break;      
      }     
      //otherwise we wait for the owner to open it       
      switch(checkTag(&piggyBank)){
        case NO_TAG_READ:
          break;
          
        case UNKNOWN_TAG_READ:
          beeper.beep(FAIL);
          displayUnknownTagAlert();    
          break;        
          
        case USER_TAG_READ:
          beeper.beep(SUCCESS);
          setStatus(&piggyBank, DOOR_OPEN); 
          displayKnownTagMessage(USER_TAG);        
          break;  
                
        case MASTER_TAG_READ:
          //user tag recovery proc.
          beeper.beep(SUCCESS);
          setStatus(&piggyBank, WAITING_NEW_USER_TAG);  
          displayKnownTagMessage(MASTER_TAG);      
          break;
      }
      break;
      
    case DOOR_OPEN:
      displayDoorOpenStatus();
      if(!piggyBank.isDoorOpen){
        setStatus(&piggyBank, ACCEPTING_COINS);  
        break;
      }    
      if(piggyBank.isTrayMisplaced){
        setStatus(&piggyBank, TRAY_ERROR);
        break;
      }
      //reset amount
      if(piggyBank.isResetAmountRequested){
        setStatus(&piggyBank, RESET_AMOUNT);
        break;
      }      
      break;

    case RESET_AMOUNT:
      displayResetAmountAlert();
      if(!piggyBank.isResetAmountRequested){
        DEBUG_PRINTLN("Reset amount cancelled");
        beeper.beep(FAIL);
        setStatus(&piggyBank, piggyBank.PreviousStatus); 
      }else if(!tagScanTimer){
        DEBUG_PRINTLN("Reset amount!");
        beeper.beep(SUCCESS);
        //Reset amount
        piggyBank.Amount = 0;
        piggyBank.NumberOfCoins[0] = 0;
        piggyBank.NumberOfCoins[1] = 0;
        piggyBank.NumberOfCoins[2] = 0;
        saveAmountToEEPROM(&piggyBank);        
        displayAmount(&piggyBank);
        //wait for btn release
        while(reset_amount_btn.isPressed())
          ;      
        //return to previuos status
        piggyBank.isResetAmountRequested = false;
        setStatus(&piggyBank, piggyBank.PreviousStatus);
        break;
      }    
      break;
              
        
    default:
      // statements
      break;
  } 
}

/***************************************************************
 * checkMasterTag
 * Note: this must be called after readRFID()
 * to ensure readCard[] contains PICC
 ***************************************************************/
bool checkMasterTag(PiggyBankType *pb){
  return (*pb).MasterSecurityCode[0] == readCard[0] &&
    (*pb).MasterSecurityCode[1] == readCard[1] &&
    (*pb).MasterSecurityCode[2] == readCard[2] &&
    (*pb).MasterSecurityCode[3] == readCard[3];
}

/***************************************************************
 * checkUserTag
 * Note: this must be called after readRFID()
 * to ensure readCard[] contains PICC
 ***************************************************************/
bool checkUserTag(PiggyBankType *pb){
  return (*pb).UserSecurityCode[0] == readCard[0] &&
    (*pb).UserSecurityCode[1] == readCard[1] &&
    (*pb).UserSecurityCode[2] == readCard[2] &&
    (*pb).UserSecurityCode[3] == readCard[3];
}

/***************************************************************
 * checkTmpTag
 * Note: this must be called after readRFID()
 * to ensure readCard[] contains PICC
 ***************************************************************/
bool checkTmpTag(){
  return tmp_readCard[0] == readCard[0] &&
    tmp_readCard[1] == readCard[1] &&
    tmp_readCard[2] == readCard[2] &&
    tmp_readCard[3] == readCard[3];
}

/***************************************************************
 * checkTag
 ***************************************************************/
TAG_READ_TYPE checkTag(PiggyBankType *pb) {
  if(readRFID()){
    if(checkUserTag(pb))
      return USER_TAG_READ;
    else if(checkMasterTag(pb))
      return MASTER_TAG_READ;
    else
      return UNKNOWN_TAG_READ;    
  }else{
    return NO_TAG_READ;
  }
}

/***************************************************************
 * acceptCoins
 * - if coin available, read and increase amount
 ***************************************************************/
inline void acceptCoins(PiggyBankType *pb){
  while(coinAcceptor.coinsAvalilable())
  {   
    // read the incoming byte from coin acceptor
    int incomingCoin = coinAcceptor.readCoin();
    int idx = (incomingCoin / COIN_50CT) >> 1; //this maps incomingCoin (e.g. 5, 10 or 20) to 0, 1, 2
    switch (incomingCoin){
      case COIN_50CT:    
      case COIN_1EUR:
      case COIN_2EUR:
        (*pb).Amount += incomingCoin;
        (*pb).NumberOfCoins[idx]++;  
        saveAmountToEEPROM(pb);
        beeper.beep(SUCCESS);
        displayCoin(incomingCoin);
        displayAmount(pb);
        displayNumberOfCoins(&piggyBank);
        break;
        
      default:
        //this is not supposed to happen
        DEBUG_PRINT(F("Unexpected read from Coin Acceptor: "));
        DEBUG_PRINTLN(incomingCoin);
        break; 
    }
  }
}

/***************************************************************
 * setStatus
 * - set new status and keep track of previuos status
 ***************************************************************/
void setStatus(PiggyBankType *pb, PiggyBankStatusType newStatus){
  DEBUG_PRINT("From status: ");
  DEBUG_PRINT((*pb).CurrentStatus);
  
  (*pb).PreviousStatus = (*pb).CurrentStatus; 
  (*pb).CurrentStatus = newStatus;
  
  DEBUG_PRINT(" to status: ");
  DEBUG_PRINTLN((*pb).CurrentStatus);

  switch (newStatus) {
    case WAITING_NEW_USER_TAG:
      rfidReadCount = 0;  
      tagScanTimer = TAG_SCAN_TIMEOUT; 
      coinAcceptor.enableCoins(false);       
      break;
             
    case WAITING_NEW_MASTER_TAG:      
      rfidReadCount = 0;      
      coinAcceptor.enableCoins(false);       
      break;
       
    case RESET_AMOUNT:   
      tagScanTimer = TAG_SCAN_TIMEOUT;  
      coinAcceptor.enableCoins(false);       
      break;
      
    case INITIALIZING:      
    case TRAY_ERROR:
      coinAcceptor.enableCoins(false);       
      break;
      
    case DOOR_OPEN:   
      coinAcceptor.enableCoins(false);
      if(!(*pb).isDoorOpen){
        DEBUG_PRINTLN(F("lock released"));
        lock.release();  
      }
      //restore LCD 1st line: amount must be visible for convenience    
      displayAmount(&piggyBank); 
      break;
     
    case ACCEPTING_COINS:    
      //restore display
      displayAmount(pb);
      displayNumberOfCoins(&piggyBank);
      coinAcceptor.enableCoins(true);
      break;
                   
    default:
      // statements
      break;
  }
}

/***************************************************************
 * displayCoin
 ***************************************************************/
void displayCoin(int incomingByte){
  char *msg;
  switch (incomingByte){
    case COIN_50CT:     
      msg = "0,5";
      break;
    case COIN_1EUR:
      msg = "1,0";
      break;
    case COIN_2EUR:
      msg = "2,0";
      break;
  }
  coinAcceptor.enableCoins(false);
  lcd.clear();
  lcd.setCursor(0,1);
  snprintf(buffer, sizeof(buffer), "   +%s0 EUR", msg);  
  lcd.print(buffer);
  delay(DISPLAY_COIN_TIMEOUT);
  coinAcceptor.enableCoins(true);
}

/***************************************************************
 * isSpaceAvailable
 ***************************************************************/
/*
unsigned int measureAvailableSpace(){
  int readings = 0;
  unsigned int sensor_reading, result = 0;
  
  for(int i = 0; i < 4; i++){
    sensor_reading = ultrasonic_sensor.measure();
    if(sensor_reading > _10cm5) continue; //any measure above 10.5 cm is an error
    result += sensor_reading;
    readings++;
  }
  
  result /= readings;
  
  DEBUG_PRINT(F("distance: "));
  DEBUG_PRINTLN(result);
  
  if(result){
    if(result >= _10cm0) result = _10cm0;
    if(result <= _5cm0) result = _5cm0;
    
    result = (unsigned int)map((long)result, (long)_5cm0, (long)_10cm0, 0L, 100L);
    
    DEBUG_PRINT(F(" Space (%): "));
    DEBUG_PRINTLN(result);
  }
  return result;
}
*/
bool isSpaceAvailable(){
  int readings = 0;
  unsigned int sensor_reading, distance = 0;
  
  for(int i = 0; i < MAX_READINGS; i++){
    sensor_reading = ultrasonic_sensor.measure();
    if(sensor_reading > _10cm5) continue; //any measure above 10.5 cm is an error
    distance += sensor_reading;
    readings++;
  }
  
  distance /= readings;
  
  //DEBUG_PRINT(F("distance: "));
  //DEBUG_PRINTLN(distance);

  return distance > _5cm0;
}

/***************************************************************
 * displayTrayErrorAlert
 ***************************************************************/
void displayTrayErrorAlert(){
  lcd.setCursor(0, 1); // Print on second  row
  if(isSpaceAvailable())
    lcd.print(heartbeat ? F("--- Verifica ---") : F("-il contenitore-"));
  else
    lcd.print(F("---- PIENO! ----"));    
}

/***************************************************************
 * displayDoorOpenStatus
 ***************************************************************/
void displayDoorOpenStatus(){
  lcd.setCursor(0, 1); // Print on second  row
  lcd.print(heartbeat ? F("--- Chiudere ---") : F("-- sportello! --"));
}

/***************************************************************
 * displayMasterCodeAlert
 ***************************************************************/
void displayMasterCodeAlert(){
  lcd.setCursor(0, 0); // Print on first row
  lcd.print(heartbeat ? F("-- Tag MASTER --") : F("- non definito -"));
  lcd.setCursor(0, 1); // Print on second  row
  snprintf(buffer, sizeof(buffer), "attesa lettura %1d", rfidReadCount + 1);  
  lcd.print(buffer);
}

/***************************************************************
 * displayUserTagAlert
 ***************************************************************/
void displayUserTagAlert(){
  lcd.setCursor(0, 0); // Print on first row
  lcd.print(heartbeat ? F("-- Avvicinare --") : F("nuovo tag UTENTE"));
  lcd.setCursor(0, 1); // Print on second  row
  snprintf(buffer, sizeof(buffer), "lettura %1d... %2ds", rfidReadCount + 1, tagScanTimer);  
  lcd.print(buffer);
}

/***************************************************************
 * displayResetAmountAlert
 ***************************************************************/
void displayResetAmountAlert(){
  lcd.setCursor(0, 0); // Print on first row
  lcd.print(F("- Azzeramento  -"));
  lcd.setCursor(0, 1); // Print on second  row
  snprintf(buffer, sizeof(buffer), "%15ds", tagScanTimer);  
  lcd.print(buffer);
}

/***************************************************************
 * displayUnknownTagAlert
 ***************************************************************/
void displayUnknownTagAlert(){
  bool restoreCoinAcceptor = coinAcceptor.isEnabled;
  if(restoreCoinAcceptor) coinAcceptor.enableCoins(false);
  LCD_Print(F("-- Attenzione --"), F("tag sconosciuto!"), DISPLAY_STATUS_TIMEOUT);
  if(restoreCoinAcceptor) coinAcceptor.enableCoins(true);
}

/***************************************************************
 * displayKnownTagMessage
 ***************************************************************/
void displayKnownTagMessage(bool master){
  bool restoreCoinAcceptor = coinAcceptor.isEnabled;
  if(restoreCoinAcceptor) coinAcceptor.enableCoins(false);
  LCD_Print(master ? F("-- Tag MASTER --") : F("-- Tag UTENTE --"), F("- riconosciuto -"), DISPLAY_STATUS_TIMEOUT);
  if(restoreCoinAcceptor) coinAcceptor.enableCoins(true);     
 }

/***************************************************************
 * LCD_Print
 * msg1: PROGMEM string display on row1
 * msg2: PROGMEM string display on row2
 * wait: delay 'wait' milliseconds after displaying text;
 *       0 = do not wait
 ***************************************************************/
void LCD_Print(const __FlashStringHelper *msg1, const __FlashStringHelper *msg2, unsigned int wait){
  LCD_Print_Row(msg1, 0);
  LCD_Print_Row(msg2, 1); 
  if(wait) delay(wait);  
}

void LCD_Print_Row(const __FlashStringHelper *msg, byte row){
  lcd.setCursor(0, row);
  lcd.print(msg);
}

/*******************************************************************
 * displayAmount
 * Note: implicit assumption that amount is less then 65536
 * (65.536,00 eur) in conversion code
 *******************************************************************/
void displayAmount(PiggyBankType *pb){
  //
  DEBUG_PRINT("EUR: ");
  DEBUG_PRINTLN((*pb).Amount);
  //
  bool pad = false;
  unsigned int result, tmp_amount = (*pb).Amount;
  
  lcd.setCursor(0,0); // Print on first row
  lcd.print(F("Tot.EUR "));
  
  result = tmp_amount / 10000;
  tmp_amount %= 10000;

  if(result){
    pad = true;
    lcd.print(result);
    lcd.print(".");
  }else{
    lcd.print("  ");
  }
  
  result = tmp_amount / 10;
  tmp_amount %= 10;
    
  snprintf(buffer, sizeof(buffer), pad ? "%03d,%d0" : "%3d,%d0", result, tmp_amount);  
  lcd.print(buffer);
}

/*******************************************************************
 * displayNumberOfCoins
 *******************************************************************/
void displayNumberOfCoins(PiggyBankType *pb){
  lcd.setCursor(0,1);
  lcd.print(F("Da "));
  switch(coinIdx){
    case 0:
      lcd.print(F("0,50"));
      break;     
    case 1:
      lcd.print(F("1,00"));
      break;     
    case 2:
      lcd.print(F("2,00"));
      break; 
  }
  snprintf(buffer, sizeof(buffer), ":%8d", (*pb).NumberOfCoins[coinIdx]);
  lcd.print(buffer);  
}

/***************************************************************
 * InitFromEEPROM
 * reads amount and security codes from EEPROM
 * initialised EEPROM layout: 
 * +---------------+-------------------------+-------------------------+-----+-----+-----+-----+-----+-----+-----+-----+
 * | '$' | X0 | X1 | '?' | U0 | U1 | U2 | U3 | '*' | M0 | M1 | M2 | M3 | '@' | C00 | C01 | C10 | C12 | C20 | C21 | '#' |
 * +---------------+-------------------------+-------------------------+-----+-----+-----+-----+-----+-----+-----+-----+
 *    0    1    2     3    4    5    6    7     8    9    10   11   12    13    14    15    16    17    18    19   20
 *    
 * Note: 
 * 1. internal EEPROM has a limited lifetime (about 100,000 write cycles)
 * 2. EEPROM: 1024 bytes on the ATmega328
 * 
 ***************************************************************/
inline void initFromEEPROM(PiggyBankType *pb){
  //if EEPROM_AMOUNT_DELIM not found, assume amount in EEPROM not initialised, so initialise
  if(EEPROM_AMOUNT_DELIM != EEPROM.read(EEPROM_ADDRESS_AMOUNT_DELIM)){
    DEBUG_PRINTLN(F("EEPROM_AMOUNT_DELIM not found"));
    EEPROM.put(EEPROM_ADDRESS_AMOUNT_DELIM, EEPROM_AMOUNT_DELIM); 
    EEPROM.put(EEPROM_ADDRESS_AMOUNT, 0);
    //
    EEPROM.put(EEPROM_ADDRESS_COINS_DELIM, EEPROM_COINS_DELIM); 
    EEPROM.put(EEPROM_ADDRESS_COIN0, 0);
    EEPROM.put(EEPROM_ADDRESS_COIN1, 0);
    EEPROM.put(EEPROM_ADDRESS_COIN2, 0);    
    //
    EEPROM.put(EEPROM_ADDRESS_END, EEPROM_END_DELIM);
  }

  //if EEPROM_MASTER_SECURITYCODE_DELIM not found, assume MASTER CODE in EEPROM not initialised, so initialise
  if(EEPROM_MASTER_SECURITYCODE_DELIM != EEPROM.read(EEPROM_ADDRESS_MASTERSECURITYCODE_DELIM)){
    DEBUG_PRINTLN(F("EEPROM_MASTER_SECURITYCODE_DELIM not found"));
    (*pb).MasterSecurityCodeDefined = false;
  }else{
    (*pb).MasterSecurityCodeDefined = true;
  }

  //if EEPROM_USERSECURITYCODE_DELIM not found, assume USER CODE in EEPROM not initialised, so initialise
  if(EEPROM_USER_SECURITYCODE_DELIM != EEPROM.read(EEPROM_ADDRESS_USERSECURITYCODE_DELIM)){
    DEBUG_PRINTLN(F("EEPROM_USER_SECURITYCODE_DELIM not found"));
    (*pb).UserSecurityCodeDefined = false;
  }else{
    (*pb).UserSecurityCodeDefined = true;
  }
  
  //read values from EEPROM
  EEPROM.get(EEPROM_ADDRESS_AMOUNT, (*pb).Amount);
  EEPROM.get(EEPROM_ADDRESS_USERSECURITYCODE, (*pb).UserSecurityCode);
  EEPROM.get(EEPROM_ADDRESS_MASTERSECURITYCODE, (*pb).MasterSecurityCode);
  EEPROM.get(EEPROM_ADDRESS_COIN0, (*pb).NumberOfCoins[0]);
  EEPROM.get(EEPROM_ADDRESS_COIN1, (*pb).NumberOfCoins[1]);
  EEPROM.get(EEPROM_ADDRESS_COIN2, (*pb).NumberOfCoins[2]);

  if((*pb).NumberOfCoins[0] < 0) (*pb).NumberOfCoins[0] = 0;
  if((*pb).NumberOfCoins[1] < 0) (*pb).NumberOfCoins[1] = 0;
  if((*pb).NumberOfCoins[2] < 0) (*pb).NumberOfCoins[2] = 0;
  

  #ifdef DEBUG
  for(int i = EEPROM_ADDRESS_START; i <= EEPROM_ADDRESS_END; i++)
  {
    DEBUG_PRINT(i);
    DEBUG_PRINT(F("->"));
    DEBUG_PRINTLN(EEPROM.read(i));
  }
  #endif
} 

/***************************************************************
 * saveAmountToEEPROM
 ***************************************************************/
inline void saveAmountToEEPROM(PiggyBankType *pb){
  EEPROM.put(EEPROM_ADDRESS_AMOUNT, (*pb).Amount);
  EEPROM.put(EEPROM_ADDRESS_COIN0, (*pb).NumberOfCoins[0]);
  EEPROM.put(EEPROM_ADDRESS_COIN1, (*pb).NumberOfCoins[1]);
  EEPROM.put(EEPROM_ADDRESS_COIN2, (*pb).NumberOfCoins[2]);
}

/***************************************************************
 * saveMasterCodeToEEPROM
 ***************************************************************/
inline void saveMasterCodeToEEPROM(PiggyBankType *pb){
  EEPROM.put(EEPROM_ADDRESS_MASTERSECURITYCODE_DELIM, EEPROM_MASTER_SECURITYCODE_DELIM);
  EEPROM.put(EEPROM_ADDRESS_MASTERSECURITYCODE, (*pb).MasterSecurityCode);
}

/***************************************************************
 * saveUserCodeToEEPROM
 ***************************************************************/
inline void saveUserCodeToEEPROM(PiggyBankType *pb){
  EEPROM.put(EEPROM_ADDRESS_USERSECURITYCODE_DELIM, EEPROM_USER_SECURITYCODE_DELIM);
  EEPROM.put(EEPROM_ADDRESS_USERSECURITYCODE, (*pb).UserSecurityCode);
}

/***************************************************************
 * readRFID
 ***************************************************************/
bool readRFID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return false;
  }
  //
  #ifdef DEBUG
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  DEBUG_PRINTLN(F("Scanned PICC's UID:"));
  for (byte i = 0; i < PICC_UID_SIZE; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    DEBUG_PRINT(" 0x");
    DEBUG_PRINTHEX(readCard[i]);
  }
  DEBUG_PRINTLN("");
  #endif
  //
  mfrc522.PICC_HaltA(); // Stop reading
  return true;
}

/***************************************************************
 * CheckRFIDReader
 *             Pin layout used:
   --------------------------------------
               MFRC522      Arduino
               Reader/PCD   Uno
   Signal      Pin          Pin
   --------------------------------------
   RST/Reset   RST          9
   SPI SS      SDA(SS)      10
   SPI MOSI    MOSI         11 / ICSP-4
   SPI MISO    MISO         12 / ICSP-1
   SPI SCK     SCK          13 / ICSP-3

   (from AccessControl.ino in miguelbalboa Github samples)
 ***************************************************************/
#ifdef DEBUG
inline void CheckRFIDReader() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  DEBUG_PRINT(F("MFRC522 Software Version: 0x"));
  DEBUG_PRINTHEX(v);
  if (v == 0x91)
    DEBUG_PRINT(F(" = v1.0"));
  else if (v == 0x92)
    DEBUG_PRINT(F(" = v2.0"));
  else
    DEBUG_PRINT(F(" (unknown)"));
  DEBUG_PRINTLN("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    DEBUG_PRINTLN(F("WARNING: Communication failure, check MFRC522 connection"));
    DEBUG_PRINTLN(F("SYSTEM HALTED: Check connections."));
    while (true); // do not go further
  }
}
#endif
