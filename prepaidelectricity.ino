#include <SoftwareSerial.h> // Arduino IDE <1.6.6
#include "PZEM004T.h"
#include <LiquidCrystal.h>
#include "GPRS_Shield_Arduino.h"
#include <Wire.h>


//GSM HARDWARE SETUP
const uint8_t  PIN_TX = 3;
const uint8_t  PIN_RX = 2;
const unsigned int BAUDRATE = 9600;
//GSM Requirements
const uint8_t MESSAGE_LENGTH = 160;
char PHONE_NUMBER[]= "09xxxxxxxxx";
char MESSAGE[] =   "System up";
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];
//END
unsigned int subscriberBalance = 0;
bool checkInbox = false;
unsigned int kiloWatt = 0;
struct Options{

    char code[6];
    unsigned int value;
};

struct Options Option1;
struct Options Option2;
struct Options Option3;

LiquidCrystal lcd(22, 28, 9, 10, 11, 32);
PZEM004T pzem(1,0);  //(RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);
GPRS GSMTEST(PIN_RX,PIN_TX,BAUDRATE);//RX,TX,BAUDRATE

void setup() {

  Serial.begin(9600);

  pzem.setAddress(ip);
  lcd.begin(20, 4); // lcd rows and columns
 
  strcpy(Option1.code,"PE100"); //assign sms code for 100KW
  Option1.value = 100;//100 KW

  strcpy(Option2.code,"PE200"); //assign sms code for 200KW
  Option2.value = 200;//200 KW

  strcpy(Option1.code,"PE300"); //assign sms code for 300KW
  Option1.value = 300;//300 KW
   
}


void loop() {
  
  checkInbox = checkInboxContent();
  if(checkInbox){
    if(message == Option1.code){
      subscriberBalance += Option1.value;
    }
    else if(message == Option2.code){
      subscriberBalance += Option2.value;
    }

    else if(message == Option3.code){
      subscriberBalance += Option3.value;
    }
    else{
      //Do nothing
    }
  }

  float v = pzem.voltage(ip);

  if (v < 0.0) v = 0.0;

  Serial.print(v);
  Serial.print("V; ");
  lcd.setCursor(0,0);
  lcd.print("V= ");
  lcd.setCursor(2,0);
  lcd.print(v);


  float i = pzem.current(ip);

  if (i < 0.0) i = 0.0;

  Serial.print(i);Serial.print("A; ");

  lcd.setCursor(9,0);
  lcd.print("A= ");
  lcd.setCursor(11,0);
  lcd.print(i);




  float p = pzem.power(ip);

  if (p < 0.0) p = 0.0;


  Serial.print(p);Serial.print("W; ");
  lcd.setCursor(9,1);
  lcd.print("W= ");
  lcd.setCursor(11,1);
  lcd.print(p);




  float e = pzem.energy(ip);

  Serial.print("PF= ");Serial.print((p)/(v*i));

  lcd.setCursor(0,1);
  lcd.print("PF=");
  lcd.setCursor(3,1);
  lcd.print((p)/(v*i));
  Serial.print("kWh= ");Serial.print(e);

  lcd.setCursor(0,2);
  lcd.print("kWh=");
  lcd.setCursor(3,2);
  lcd.print((e/1000));

  
  Serial.println();

  

  delay(100);
}

bool checkInboxContent(){

   messageIndex = GSMTEST.isSMSunread();
   if (messageIndex > 0) { //AT LEAST, THERE IS ONE UNREAD SMS
      GSMTEST.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);           
      //IN ORDER NOT TO FUL SIM Memory, IS BETTER TO DELETE IT
      GSMTEST.deleteSMS(messageIndex);
//      //Serial.print("FROM NUMBER: ");
//      Serial.println(phone);  
//      //Serial.print("DATE/TIME");
//      Serial.println(datetime);        
//      //Serial.print("RECEIVED MESSAGE: ");
//      Serial.println(message);

        return true;

   }
  return false;
}
