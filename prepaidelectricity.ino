#include <SoftwareSerial.h> // Arduino IDE <1.6.6
#include "PZEM004T.h"
#include "LiquidCrystal_I2C.h" 
#include "GPRS_Shield_Arduino.h"
#include <Wire.h>
#include <EEPROM.h>     // We are going to read and write to EEPROM
#include <stdlib.h>
uint8_t extracted_stripednum[5]={0,0,0,0,0};
//GSM HARDWARE SETUP
#define  PIN_TX  53
#define  PIN_RX  52
#define  BAUDRATE  9600
//GSM Requirements
const uint8_t MESSAGE_LENGTH = 160;
const uint8_t PHONE_LENGTH = 11;
char PHONE_NUMBER[]= "09567389688";
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
bool checkInbox = false;
float kiloWatt = 0.0f;
float kiloWattPrev = 0.0f;
struct Options{
    char code[6];
    float value = 0.0;
};
const int relaypin = 7;
struct Options Option1;
struct Options Option2;
struct Options Option3;
bool half = false;
LiquidCrystal_I2C lcd(0x27,20,4);
GPRS GSMTEST(PIN_RX,PIN_TX,BAUDRATE);//RX,TX,BAUDRATE
PZEM004T pzem(50,51);  //(RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);


void setup() {
  pinMode(relaypin,OUTPUT);
  Serial.begin(9600);
  if(isnan(EEPROM.get(0xF,subscriberBalance))){
    subscriberBalance = 0.0;
  }
  else{
     subscriberBalance = EEPROM.get(0xF,subscriberBalance);
     if(subscriberBalance > 0.0){
      ON();
     }
     else{
      OFF();
     }
  }

//  if((EEPROM.get(0xF,subscriberBalance) > 100000)){
//    subscriberBalance = 0.0;
//  }

  if(isnan(EEPROM.get(0x16,kiloWattPrev))){
    kiloWattPrev = 0.0;
  }
  else{
    kiloWattPrev = EEPROM.get(0x16,kiloWattPrev);
  }
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  strcpy(Option1.code,"PE1"); //assign sms code for 100KW
  Option1.value = 1.0;//100 KW
  
  strcpy(Option2.code,"PE2"); //assign sms code for 200KW
  Option2.value = 2.0;//200 KW

  strcpy(Option3.code,"PE3"); //assign sms code for 300KW
  Option3.value = 3.0;//300 KW
  lcd.clear();
  lcd.print(F("Initializing"));
  while(!GSMTEST.init()) {
      delay(10000L);
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
          EEPROM.put(0xF,subscriberBalance);
        }
        else if(strcmp(message,Option2.code)==0){
          subscriberBalance += Option2.value;
          EEPROM.put(0xF,subscriberBalance);
        }
    
        else if(strcmp(message,Option3.code)==0){
          subscriberBalance += Option3.value;
          EEPROM.put(0xF,subscriberBalance);
 
          
        }

       else if(strcmp(message,"PE1 set")==0){
          subscriberBalance = Option1.value;
          EEPROM.put(0xF,subscriberBalance);
        }
      else if(strcmp(message,"PE set1")==0){
          subscriberBalance = 0.1;
          EEPROM.put(0xF,subscriberBalance);
        }
     else if(strcmp(message,"PE set2")==0){
          subscriberBalance = 0.2;
          EEPROM.put(0xF,subscriberBalance);
        }
     else if(strcmp(message,"PE reset")==0){
          subscriberBalance = 0.0;
          EEPROM.put(0xF,subscriberBalance);
        }          
        else{
         Serial.println(codeError);
        }
        Serial.println(subscriberBalance);
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
    }
    
    EEPROM.put(0xF,subscriberBalance);
    kiloWattPrev = kiloWatt;
    EEPROM.put(0x16,kiloWattPrev);
    lcd.setCursor(12,3);
    lcd.print(subscriberBalance);
    if(subscriberBalance <= 0.5*subscriberBalance && subscriberBalance > 0.0){
      if(!half){
        half = true;
        GSMTEST.sendSMS(PHONE_NUMBER,BALANCE_HALF);
      }
    }
    else{
      if(half){
        half = false;
      }
    }
   if(subscriberBalance > 0.0){
    ON();
   }
     else {
      OFF();
      subscriberBalance = 0.0;
      EEPROM.put(0xF,subscriberBalance);
     }
  }
  lcd.setCursor(3,1);
  lcd.print((p)/(v*i));
  lcd.setCursor(4,2);
  //lcd.print((e/1000.0));
  lcd.print(kiloWatt);
  delay(1000);


  
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
    }while (!checkInboxContent());                  // Program will not go further while you not get a successful read

    if(strcmp(message,"reg 143")==0){

             convertNumber();
            for ( int j = 0; j < 5; j++ ) {        // Loop 4 times
              EEPROM.write( 2 + j, extracted_stripednum[j] );  // Write scanned PICC's UID to EEPROM, start from address 2
            }
            EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
            lcd.clear();
            lcd.print(F("Master Defined"));
         
          lcd.clear();
          lcd.print(F("Master Number"));
          for ( int i = 0; i < 5; i++ ) {          // Read Master Card's UID from EEPROM
            extracted_stripednum[i] = EEPROM.read(2 + i);    // Write it to masterCard
          }
          //Serial.println();
          for(int i=0;i< 5;i++){
            char tempbuffer[2];
            static int counter = 4;
            if(i<4){
              //MASTER_NUMBER[i] = extracted_stripednum[i];
              
              itoa(extracted_stripednum[i],tempbuffer,10);
              MASTER_NUMBER[counter++] = tempbuffer[0];
              MASTER_NUMBER[counter++] = tempbuffer[1];
        
        //      Serial.println(tempbuffer[0]);
        //      Serial.println(tempbuffer[1]);
            }
            else{
              itoa(extracted_stripednum[i],tempbuffer,10);
              MASTER_NUMBER[counter++] = tempbuffer[0];
            }
            
          }
         for(int i=0;i<strlen(MASTER_NUMBER);i++){
          //Serial.print(MASTER_NUMBER[i]);
         }
         //Serial.println();
         lcd.setCursor(0,1);
         lcd.print(MASTER_NUMBER);
         GSMTEST.sendSMS(MASTER_NUMBER,MESSAGE);
         lcd.clear(); 
         defaultLCD();

    }

    else{
      lcd.clear();
      CheckMaster();
    }

    }

    else{

      for ( int i = 0; i < 5; i++ ) {          // Read Master Card's UID from EEPROM
            extracted_stripednum[i] = EEPROM.read(2 + i);    // Write it to masterCard
          }
          //Serial.println();
          for(int i=0;i< 5;i++){
            char tempbuffer[2];
            static int counter = 4;
            if(i<4){
              //MASTER_NUMBER[i] = extracted_stripednum[i];
              
              itoa(extracted_stripednum[i],tempbuffer,10);
              MASTER_NUMBER[counter++] = tempbuffer[0];
              MASTER_NUMBER[counter++] = tempbuffer[1];
        
        //      Serial.println(tempbuffer[0]);
        //      Serial.println(tempbuffer[1]);
            }
            else{
              itoa(extracted_stripednum[i],tempbuffer,10);
              MASTER_NUMBER[counter++] = tempbuffer[0];
            }
            
          }
         for(int i=0;i<strlen(MASTER_NUMBER);i++){
          //Serial.print(MASTER_NUMBER[i]);
         }
         //Serial.println();
         lcd.setCursor(0,1);
         lcd.print(MASTER_NUMBER);
         GSMTEST.sendSMS(MASTER_NUMBER,MESSAGE);
         lcd.clear(); 
         defaultLCD();
    }

 
}

void convertNumber(){
  char stripednum[9];
  int increment = 0;
  char numlength = strlen(phone);

  if(numlength > 11){
    increment = 4;
  }
  else{
    increment = 2;
  }
  for(int i=0;i<numlength;i++) if(i<=9) stripednum[i] = phone[i+increment];
 for(int i=0;i<5;i++){
        static int counter = 0;
        if(counter < 8){
            if(extracted_stripednum[i] == 0) extracted_stripednum[i] = (stripednum[counter] - 48)* 10; // multiply by ten for tens place
              extracted_stripednum[i] += (stripednum[counter+1] - 48) * 1; // multiply by one for ones place
              counter += 2;
        }

        else{
          if(extracted_stripednum[i] == 0) extracted_stripednum[i] = (stripednum[counter] - 48)* 1; // multiply by ten for tens place
        }
        
        //Serial.println((byte)extracted_stripednum[i]);
      
 }
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
  lcd.print(F("kWh="));
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
