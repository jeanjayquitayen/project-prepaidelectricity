#include <SoftwareSerial.h> // Arduino IDE <1.6.6
#include "PZEM004T.h"
#include "LiquidCrystal_I2C.h" 
#include "GPRS_Shield_Arduino.h"
#include <Wire.h>
#include <EEPROM.h>     // We are going to read and write to EEPROM
#include <stdlib.h>
#define subscriberBalanceADDR 0xF
#define kiloWattPrevADDR 0x16

//GSM HARDWARE SETUP
#define  PIN_TX  53
#define  PIN_RX  52
#define  BAUDRATE  9600

//GSM Requirements
const uint8_t MESSAGE_LENGTH = 160;
const uint8_t PHONE_LENGTH = 11;
char MASTER_NUMBER[] = "+639xxxxxxxxx"; //template , do not change
char MESSAGE[] =   "System up";
char myreply[] =   "message received!";
char codeError[] = "CODE DID NOT MATCH";
char notMASTER[] = "Sorry, Number not Allowed!";
char BALANCE_HALF[] = "You have consumed 50% of your Power Allowance!";
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];
//END
float subscriberBalance = 0.0f;
float subscriberPoints = 0.0f;
bool checkInbox = false;
float kiloWatt = 0.0f;
float kiloWattPrev = 0.0f;
struct Options{
    char code[6];
    float value = 0.0;
};
const int relaypin = 7;
const int buzzerpin = 9;
struct Options Option1;
struct Options Option2;
struct Options Option3;
bool half = false;
bool on = false;
int toggle = 0;
LiquidCrystal_I2C lcd(0x27,20,4);
GPRS GSMTEST(PIN_RX,PIN_TX,BAUDRATE);//RX,TX,BAUDRATE
PZEM004T pzem(50,51);  //(RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);

void setup() {
  pinMode(relaypin,OUTPUT);
  pinMode(8,OUTPUT);
  pinMode(buzzerpin,OUTPUT);
  digitalWrite(8,LOW); //pin8 is used as ground or common
  digitalWrite(relaypin,LOW);
  digitalWrite(buzzerpin,LOW);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  if(isnan(EEPROM.get(subscriberBalanceADDR,subscriberBalance))){
    subscriberBalance = 0.0;
  }
  else{
     subscriberBalance = EEPROM.get(subscriberBalanceADDR,subscriberBalance);
//     if(subscriberBalance > 0.0){
//      ON();
//     }
//     else{
//      OFF();
//     }
  }

  if(isnan(EEPROM.get(kiloWattPrevADDR,kiloWattPrev))){
    kiloWattPrev = 0.0;
  }
  else{
    kiloWattPrev = EEPROM.get(kiloWattPrevADDR,kiloWattPrev);
  }

  strcpy(Option1.code,"PE1"); //assign sms code for 100KW
  Option1.value = 1.0;//100 KW
  strcpy(Option2.code,"PE2"); //assign sms code for 200KW
  Option2.value = 2.0;//200 KW
  strcpy(Option3.code,"PE3"); //assign sms code for 300KW
  Option3.value = 3.0;//300 KW
  strcpy(Option1.code,"PE1"); //assign sms code for 100KW
  Option1.value = 1.0;//100 KW
  strcpy(Option2.code,"PE2"); //assign sms code for 200KW
  Option2.value = 2.0;//200 KW
  strcpy(Option3.code,"PE3"); //assign sms code for 300KW
  Option3.value = 3.0;//300 KW
  lcd.clear();
  lcd.print(F("Initializing"));
  while(!GSMTEST.init()){
      delay(1000);
      //digitalWrite(7,LOW);
      lcd.setCursor(0,1);
      lcd.print(F("INIT ERROR"));
      lcd.setCursor(0,2);
      lcd.print(F("Check GSM Module"));
      lcd.setCursor(0,3);
      lcd.print(F("Retrying..."));
  }
  pzem.setAddress(ip);
  lcd.clear(); 
  CheckMaster();
  lcd.clear(); 
  defaultLCD();
}
void loop() {
  checkInbox = checkInboxContent();
    if(checkInbox){
      if(strcmp(phone,MASTER_NUMBER) == 0){
        if(strcmp(message,Option1.code)==0){
          subscriberBalance += Option1.value;
          subscriberPoints += Option1.value;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
        else if(strcmp(message,Option2.code)==0){
          subscriberBalance += Option2.value;
          subscriberPoints += Option2.value;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
    
        else if(strcmp(message,Option3.code)==0){
          subscriberBalance += Option3.value;
          subscriberPoints += Option3.value;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);   
        }

       else if(strcmp(message,"PE1 set")==0){
          subscriberBalance = Option1.value;
          subscriberPoints = Option1.value;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
       else if(strcmp(message,"PE set1")==0){
          subscriberBalance = 0.1;
          subscriberPoints = 0.1;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
      else if(strcmp(message,"PE set2")==0){
          subscriberBalance = 0.2;
          subscriberPoints = 0.2;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
      else if(strcmp(message,"PE reset")==0){
          subscriberBalance = 0.0;
          subscriberPoints = 0.0;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,subscriberBalance);
        }
          
    else{
         Serial.println(codeError);
         GSMTEST.sendSMS(MASTER_NUMBER,codeError);
        }
    }
  else if(strcmp(message,"delete master")==0){
          EEPROM.write(1,0);
          lcd.clear();
          CheckMaster();
        }

 else{
      Serial.println(notMASTER);
    }

    defaultLCD();
}

 if(subscriberBalance > 0.0){
    if(!on){
      on = true;
      ON();
    }
    
   }
else {
    if(on){
    for(int i = 0;i<10;i++){
      digitalWrite(buzzerpin,toggle ^= 1);
      delay(100);
    }
      digitalWrite(buzzerpin,LOW);
    
    on = false;
    subscriberBalance = 0.0;
    EEPROM.put(subscriberBalanceADDR,subscriberBalance);
    subscriberPoints = 0.0;
    EEPROM.put(0x30,subscriberPoints);
    EEPROM.put(subscriberBalanceADDR,0.0);
    OFF();
    }

  }
  
if(subscriberBalance <= 0.5*subscriberPoints){
      if(!half){
        half = true;
        GSMTEST.sendSMS(MASTER_NUMBER,BALANCE_HALF);
        if(on){
        for(int i = 0;i<10;i++){
          digitalWrite(buzzerpin,toggle ^= 1);
          delay(1000);
        }
      digitalWrite(buzzerpin,LOW);
    
    on = false;
      }
    }
else{
      if(half){
        half = false;
        on = true;
      }
    }

  float v = pzem.voltage(ip);
  if (v < 0.0) v = 0.0;
  lcd.setCursor(2,0);
  lcd.print(v);
  float i = pzem.current(ip);
  if (i < 0.0) i = 0.0;
  lcd.setCursor(11,0);
  lcd.print(i);
  float p = pzem.power(ip);
  if (p < 0.0) p = 0.0;
  lcd.setCursor(11,1);
  lcd.print(p);
  float e = pzem.energy(ip);
  kiloWatt = e/1000.0;
  if(kiloWatt !=  kiloWattPrev){ //check if reading has changed
    if(subscriberBalance > 0.0){
      subscriberBalance -= (kiloWatt - kiloWattPrev);
      EEPROM.put(subscriberBalanceADDR,subscriberBalance);
    }
    else{
          subscriberPoints = 0.0;
          EEPROM.put(0x30,subscriberPoints);
          EEPROM.put(subscriberBalanceADDR,0.0);
    }
 
    kiloWattPrev = kiloWatt;
    EEPROM.put(kiloWattPrevADDR,kiloWattPrev);
    lcd.setCursor(12,3);
    lcd.print(subscriberBalance);

  }
  lcd.setCursor(3,1);
  float pf =(p)/(v*i);
  if(pf<=1){
    lcd.print(pf);
  }

  lcd.setCursor(11,2);
  lcd.print(kiloWatt);
  delay(1000);  
  }
}

bool checkInboxContent(){
   messageIndex = GSMTEST.isSMSunread();
   if (messageIndex > 0) { //AT LEAST, THERE IS ONE UNREAD SMS
      GSMTEST.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);           
      //IN ORDER NOT TO FUL SIM Memory, IS BETTER TO DELETE IT
      GSMTEST.deleteSMS(messageIndex);
      Serial.print("FROM NUMBER: ");
      Serial.println(phone);  
      Serial.print("DATE/TIME");
      Serial.println(datetime);        
      Serial.print("RECEIVED MESSAGE: ");
      Serial.println(message);
        return true;
   }
  return false;
}

void CheckMaster(){
   if (EEPROM.read(1) != 143) {
    lcd.print(F("No Master Defined"));
    lcd.setCursor(0,1);
    lcd.print(F("Reg Num"));
    do {
      //do nothing
    }while (!checkInboxContent());            // Program will not go further while you not get a successful read
    if(strcmp(message,"reg 143")==0){
         EEPROM.put(2,phone);
         delay(100);
         EEPROM.write(1,143);
         delay(100);
         EEPROM.get(2,MASTER_NUMBER);
         Serial.print(MASTER_NUMBER);
         Serial.println();
         //GSMTEST.sendSMS(MASTER_NUMBER,MESSAGE);
    }

    else{
         lcd.clear();
         CheckMaster();
    }
  }
 EEPROM.get(2,MASTER_NUMBER);
 lcd.clear();
 delay(100);
 lcd.clear();
 lcd.setCursor(0,1);
 lcd.print(MASTER_NUMBER);
 delay(3000);
 lcd.clear();
 delay(100);
 GSMTEST.sendSMS(MASTER_NUMBER,MESSAGE);
 lcd.clear(); 
 defaultLCD();
}


void defaultLCD(){
  lcd.setCursor(0,0);
  lcd.print(F("V= "));
  lcd.setCursor(9,0);
  lcd.print(F("A= "));
  lcd.setCursor(9,1);
  lcd.print(F("W= "));
  lcd.setCursor(0,1);
  lcd.print(F("PF="));
  lcd.setCursor(0,2);
  lcd.print(F("Cons.(kWh)="));
  lcd.setCursor(0,3);
  lcd.print(F("E-Remaining:"));
  lcd.setCursor(12,3);
  lcd.print(subscriberBalance);
  lcd.setCursor(17,3);
  lcd.print("KWh");
}

void ON(){
  digitalWrite(relaypin,HIGH);
}

void OFF(){
  digitalWrite(relaypin,LOW);
}
