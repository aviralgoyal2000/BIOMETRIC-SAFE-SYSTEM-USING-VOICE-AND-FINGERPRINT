#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial myserial(10,9);

#else
#define myserial Serial1

#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&myserial);
uint8_t id;

VR myVR(6,7);    // 6:RX 7:TX, you can choose your favourite pins.

uint8_t records[7]; // save record //default
uint8_t buf[64];

#define codeword    (20)

LiquidCrystal lcd(12,11,5,4,3,2);


void printSignature(uint8_t *buf, int len)
{
  int i;
  for(i=0; i<len; i++)
  {
    if(buf[i]>0x19 && buf[i]<0x7F)
    {
      Serial.write(buf[i]);
    }
    else
    {
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}


void printVR(uint8_t *buf)
{
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");
  
  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if(buf[0] == 0xFF)
  {
    Serial.print("NONE");
  }
  else if(buf[0]&0x80)
  {
    Serial.print("UG ");
    Serial.print(buf[0]&(~0x80), DEC);
  }
  else
  {
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");
  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  
  if(buf[3]>0)
  {
    printSignature(buf+4, buf[3]);
  }
  else
  {
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}

void setup() 
{
  // put your setup code here, to run once:
  lcd.begin(16,2);
  Serial.begin(115200);
  
  Serial.println(F("\n\nAdafruit Fingerprint sensor enrollment"));
  finger.begin(57600);
  if (finger.verifyPassword()) 
  {
    Serial.println(F("Found fingerprint sensor!"));
  } 
  else 
  {
    Serial.println(F("Did not find fingerprint sensor :("));
    while(1){delay(1);}
  }
}

uint8_t readnumber(void) 
{
  uint8_t num = 0;
  
  while (num == 0) 
  {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()                     // run over and over again
{
  Serial.println("\nEnter Input:\n");
  lcd.clear();
  lcd.print("1. SET");
  delay(2000);
  lcd.clear();
  lcd.print("2. OPEN");
  delay(2000);
  lcd.clear();
  lcd.print("3. RESET");
  delay(2000);
  
  int n = Serial.parseInt();
  boolean check = true; 
  if(n==1)
  {
    lcd.clear();
    lcd.print("enroll fingerprint!");
    Serial.println(F("Please type in the ID # (from 1 to 127) you want to save this finger as..."));
    id = readnumber();
    if (id == 0) 
    {// ID #0 not allowed, try again!
      return;
    }
    Serial.print(F("Enrolling ID #"));
    Serial.println(id);
    
    while(!getFingerprintEnroll());
    lcd.clear();
    lcd.print("Fingerprint stored");
    delay(5000);

    lcd.clear();
    lcd.print("Store Voice");
    delay(4000);
  }
  else if (n==2)
  {
    lcd.clear();
    lcd.print("Fingerprint verification");
    finger.getTemplateCount();
    Serial.print(F("Sensor contains ")); Serial.print(finger.templateCount); Serial.println(F(" templates"));
    Serial.println(F("Waiting for valid finger..."));    
    while(!getFingerprintIDez());

    myVR.begin(9600);
    Serial.println(F("\nElechouse Voice Recognition V3 Module\r\nIOT Safe System Project"));
    if(myVR.clear()==0)
    {
      Serial.println(F("Recognizer cleared."));
    }
    else
    {
      Serial.println(F("Not find VoiceRecognitionModule."));
      Serial.println(F("Please check connection and restart Arduino."));
      while(1);
    }
    if(myVR.load((uint8_t)codeword)>=0)
    {
      Serial.println(F("Codeword Loaded"));
    }
    
    Serial.println(F("\nWaiting for valid voice command..."));
    lcd.clear();
    lcd.print("Voice Verification");
    while(check)
    {
      //delay(100);
      int ret;
      ret = myVR.recognize(buf, 50);
      
      if(ret>0)
      {
        switch(buf[1])
        {
          case codeword:
            lcd.clear();
            lcd.print("Voice Matched");
            delay(5000);
            check = false;
            break;
          default:
            lcd.clear();
            lcd.print("Voice Error");
            delay(3000);
            break;
        }
        /** voice recognized */
        Serial.println("Voice Matched");
        printVR(buf);
      }
    }
    lcd.clear();
    lcd.print("Safe Open");
    delay(10000);
  }
  else if(n==3)
  {
    Serial.println("System Resetted");
    lcd.clear();
    lcd.print("System");
    delay(1000);
    lcd.clear();
    lcd.print("Resetted");
    delay(2000);
  }
}

uint8_t getFingerprintEnroll() 
{
  int p = -1;
  while (p != FINGERPRINT_OK) 
  {
    p = finger.getImage();
    switch (p) 
    {
      case FINGERPRINT_OK:
      Serial.println(F("Image taken"));
      break;
      case FINGERPRINT_NOFINGER:
      Serial.println(F("."));
      break;
      case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      break;
      case FINGERPRINT_IMAGEFAIL:
      Serial.println(F("Imaging error"));
      break;
      default:
      Serial.println(F("Unknown error"));
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) 
  {
    case FINGERPRINT_OK:
    Serial.println(F("Image converted"));
    break;
    case FINGERPRINT_IMAGEMESS:
    Serial.println(F("Image too messy"));
    return p;
    case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println(F("Communication error"));
    return p;
    case FINGERPRINT_FEATUREFAIL:
    Serial.println(F("Could not find fingerprint features"));
    return p;
    case FINGERPRINT_INVALIDIMAGE:
    Serial.println(F("Could not find fingerprint features"));
    return p;
    default:
    Serial.println(F("Unknown error"));
    return p;
  }

  Serial.println(F("Remove finger"));
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) 
  {
    p = finger.getImage();
  }
  Serial.print(F("ID ")); Serial.println(id);
  p = -1;
  Serial.println(F("Place same finger again"));
  while (p != FINGERPRINT_OK) 
  {
    p = finger.getImage();
    switch (p) 
    {
      case FINGERPRINT_OK:
      Serial.println(F("Image taken"));
      break;
      case FINGERPRINT_NOFINGER:
      Serial.print(F("."));
      break;
      case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      break;
      case FINGERPRINT_IMAGEFAIL:
      Serial.println(F("Imaging error"));
      break;
      default:
      Serial.println(F("Unknown error"));
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) 
  {
    case FINGERPRINT_OK:
    Serial.println(F("Image converted"));
    break;
    case FINGERPRINT_IMAGEMESS:
    Serial.println(F("Image too messy"));
    return p;
    case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println(F("Communication error"));
    return p;
    case FINGERPRINT_FEATUREFAIL:
    Serial.println(F("Could not find fingerprint features"));
    return p;
    case FINGERPRINT_INVALIDIMAGE:
    Serial.println(F("Could not find fingerprint features"));
    return p;
    default:
    Serial.println(F("Unknown error"));
    return p;
  }

  // OK converted!
  Serial.print(F("Creating model for #"));  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) 
  {
    Serial.println(F("Prints matched!"));
  } 
  else if (p == FINGERPRINT_PACKETRECIEVEERR) 
  {
    Serial.println(F("Communication error"));
    return p;
  } 
  else if (p == FINGERPRINT_ENROLLMISMATCH) 
  {
    Serial.println(F("Fingerprints did not match"));
    return p;
  } 
  else 
  {
    Serial.println(F("Unknown error"));
    return p;
  }

  Serial.print(F("ID ")); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) 
  {
    Serial.println(F("Stored!"));
  } 
  else if (p == FINGERPRINT_PACKETRECIEVEERR) 
  {
    Serial.println(F("Communication error"));
    return p;
  } 
  else if (p == FINGERPRINT_BADLOCATION) 
  {
    Serial.println(F("Could not store in that location"));
    return p;
  } 
  else if (p == FINGERPRINT_FLASHERR) 
  {
    Serial.println(F("Error writing to flash"));
    return p;
  } 
  else 
  {
    Serial.println(F("Unknown error"));
    return p;
  }

  return true;
}

boolean getFingerprintIDez() 
{
  uint8_t p = finger.getImage();
  if(p!=FINGERPRINT_OK)  return false;
  
  p = finger.image2Tz();
  if(p!=FINGERPRINT_OK)  return false;
  
  p = finger.fingerFastSearch();
  if(p!=FINGERPRINT_OK)  return false;
  
  // found a match!
  lcd.clear();
  lcd.print("Fingerprint Matched");
  delay(2000);
  
  Serial.print(F("Found ID #")); Serial.print(finger.fingerID); 
  Serial.print(F(" with confidence of ")); Serial.println(finger.confidence);
  return true; 
}
