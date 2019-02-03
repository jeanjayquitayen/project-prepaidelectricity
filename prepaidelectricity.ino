#include <SoftwareSerial.h> // Arduino IDE <1.6.6
#include "PZEM004T.h"
#include "LiquidCrystal_I2C.h" 
#include "GPRS_Shield_Arduino.h"
#include <Wire.h>
#include <EEPROM.h>     // We are going to read and write to EEPROM
#include <stdlib.h>
uint8_t extracted_stripednum[5]={0,0,0,0,0};
//GSM HARDWARE SETUP
const uint8_t  PIN_TX = 11;
const uint8_t  PIN_RX = 10;
const unsigned int BAUDRATE = 9600;
//GSM Requirements
const uint8_t MESSAGE_LENGTH = 160;
const uint8_t PHONE_LENGTH = 11;
char PHONE_NUMBER[]= "09567389688";
char MASTER_NUMBER[] = "+639xxxxxxxxx"; //template , do not change
char MESSAGE[] =   "System up";
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];
//END
unsigned int subscriberBalance = 0;
bool checkInbox = false;
unsigned int kiloWatt = 0;
unsigned int kiloWattPrev = 0;
struct Options{

    char code[6];
    unsigned int value;
};

struct Options Option1;
struct Options Option2;
struct Options Option3;

LiquidCrystal_I2C lcd(0x27,20,4);
//PZEM004T pzem(1,0);  //(RX,TX) connect to TX,RX of PZEM
//IPAddress ip(192,168,1,1);
GPRS GSMTEST(PIN_RX,PIN_TX,BAUDRATE);//RX,TX,BAUDRATE

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.print("hello");
  Serial.begin(9600);

  //pzem.setAddress(ip);

 
  strcpy(Option1.code,"PE100"); //assign sms code for 100KW
  Option1.value = 100;//100 KW
  
  strcpy(Option2.code,"PE200"); //assign sms code for 200KW
  Option2.value = 200;//200 KW

  strcpy(Option3.code,"PE300"); //assign sms code for 300KW
  Option3.value = 300;//300 KW
  while(!GSMTEST.init()) {
      delay(1000);
      //digitalWrite(7,LOW);
      Serial.print("INIT ERROR\r\n");
  } 
  Serial.print("Initialization Successful!");
  
  CheckMaster();
}


void loop() {
  
  checkInbox = checkInboxContent();
  
    if(checkInbox){
      if(strcmp(phone,MASTER_NUMBER) == 0){
        if(strcmp(message,Option1.code)==0){
          subscriberBalance += Option1.value;
        }
        else if(strcmp(message,Option2.code)==0){
          subscriberBalance += Option2.value;
        }
    
        else if(strcmp(message,Option3.code)==0){
          subscriberBalance += Option3.value;
        }
        else{
         Serial.println("CODE DID NOT MATCH");
        }
        Serial.println(subscriberBalance);
    }
    else{
      Serial.println("SOrry Number not Allowed!");
    }
  }

//
//  float v = pzem.voltage(ip);
//
//  if (v < 0.0) v = 0.0;
//
//  Serial.print(v);
//  Serial.print("V; ");
//  lcd.setCursor(0,0);
//  lcd.print("V= ");
//  lcd.setCursor(2,0);
//  lcd.print(v);
//
//
//  float i = pzem.current(ip);
//
//  if (i < 0.0) i = 0.0;
//
//  Serial.print(i);Serial.print("A; ");
//
//  lcd.setCursor(9,0);
//  lcd.print("A= ");
//  lcd.setCursor(11,0);
//  lcd.print(i);




//  float p = pzem.power(ip);
//
//  if (p < 0.0) p = 0.0;
//
//
//  Serial.print(p);Serial.print("W; ");
//  lcd.setCursor(9,1);
//  lcd.print("W= ");
//  lcd.setCursor(11,1);
//  lcd.print(p);
//
//
//
//
//  float e = pzem.energy(ip);
//  kiloWatt = e/1000;
//  if(kiloWatt !=  kiloWattPrev){ //check if reading has change
//    subscriberBalance -= kiloWatt;
//    kiloWattPrev = kiloWatt;
//  }
//  Serial.print("PF= ");Serial.print((p)/(v*i));
//
//  lcd.setCursor(0,1);
//  lcd.print("PF=");
//  lcd.setCursor(3,1);
//  lcd.print((p)/(v*i));
//  Serial.print("kWh= ");Serial.print(e);
//
//  lcd.setCursor(0,2);
//  lcd.print("kWh=");
//  lcd.setCursor(3,2);
////  lcd.print((e/1000));
//  lcd.print(kiloWatt);

  
  //Serial.println();

  

  delay(1000);
}

bool checkInboxContent(){

   messageIndex = GSMTEST.isSMSunread();
   if (messageIndex > 0) { //AT LEAST, THERE IS ONE UNREAD SMS
      GSMTEST.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);           
      //IN ORDER NOT TO FUL SIM Memory, IS BETTER TO DELETE IT
      GSMTEST.deleteSMS(messageIndex);
      //Serial.print("FROM NUMBER: ");
      Serial.println(phone);  
      //Serial.print("DATE/TIME");
      Serial.println(datetime);        
      //Serial.print("RECEIVED MESSAGE: ");
      Serial.println(message);

        return true;

   }
  return false;
}



void CheckMaster(){
   if (EEPROM.read(1) != 143) {
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Reg Num"));
    do {
      //do nothing
    }while (!checkInboxContent());                  // Program will not go further while you not get a successful read
    convertNumber();
    for ( int j = 0; j < 5; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, extracted_stripednum[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    Serial.println(F("Master Card Defined"));
    //lcd.print("Master Card Defined");
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Master Number"));
  for ( int i = 0; i < 5; i++ ) {          // Read Master Card's UID from EEPROM
    extracted_stripednum[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(extracted_stripednum[i]);
    
  }
  Serial.println();
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
  Serial.print(MASTER_NUMBER[i]);
 }
 Serial.println();
 GSMTEST.sendSMS(MASTER_NUMBER,MESSAGE);  
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
        
        Serial.println((byte)extracted_stripednum[i]);
      
 }
}
