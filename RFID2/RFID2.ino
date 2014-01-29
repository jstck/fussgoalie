#include <MFRC522.h>
#include <SPI.h>

#define NUM_READERS 4

MFRC522 *rfids[NUM_READERS];

//Chip select pins of the different RFID reader devices
int readers_sda[NUM_READERS] = { 30, 31, 32, 33};

//int readers_sda[4] = { 32, 33, 30, 31};


const int resetPin = 22; //Actual reset pin
const int fakeResetPin = 7; //Unused reset pin we tell the MCRF522 class to use

const int interruptPin = 21; //Shared interrupt pin
const int interruptNum = 2;  //Interrupt number matching above pin

const int reasonableDelay = 25;
const int unreasonableDelay = 250;

const int resetDelay = 100;

#define CHIP_ACTIVE LOW
#define CHIP_INACTIVE HIGH

void DisableAllRFID(boolean initPins=false)
{
  //Set all reader pins to output, high (to disable all readers)
  for(int i=0; i<NUM_READERS; i++)
  {
    if(initPins)
    {
      pinMode(readers_sda[i],OUTPUT);
    }
    digitalWrite(readers_sda[i], CHIP_INACTIVE);     
  }  
}

void setup()
{
  Serial.begin(115200);
  DisableAllRFID(true);

  Serial.println("SPI");
  SPI.begin();

  Serial.println("Reset");
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, LOW);
  delay(resetDelay);
  digitalWrite(resetPin, HIGH);
  delay(resetDelay);


  for(int i=0; i<NUM_READERS; i++)
  {
    Serial.print("Create ");
    Serial.println(i);
    rfids[i] = new MFRC522(readers_sda[i], fakeResetPin);
    delay(reasonableDelay);
  }
  DisableAllRFID(true); //Disable stuff again in case we're with the broken MFRC522 library
  
  for(int i=0; i<NUM_READERS; i++)
  {
    Serial.print("Init ");
    Serial.println(i);
    rfids[i]->PCD_Init();
    delay(reasonableDelay);
  }
  DisableAllRFID();
//  pinMode(interruptPin, INPUT);
//  attachInterrupt(interruptNum, irq_rfid, CHANGE);
  Serial.println("Ready");
}

void irq_rfid()
{
  Serial.println("IRQ!");
}

void loop()
{  
  MFRC522 *rfid;
    
  for(int i=0; i<NUM_READERS; i++)
  {
    rfid=rfids[i];
//    DisableAllRFID();
//    digitalWrite(readers_sda[i], CHIP_ACTIVE);     

    delay(reasonableDelay);
    
//    Serial.print(i);
//    Serial.print(": ");
//    for(int reg=0x00; reg<=0x0E; reg++)
//    {
//      unsigned char c = rfid->PCD_ReadRegister(reg << 1);
//      if(c<0x10) Serial.print("0");
//      Serial.print(c, HEX);
//      Serial.print(" "); 
//    }
//    Serial.println();
//    delay(reasonableDelay);
  
    if( rfid->PICC_IsNewCardPresent() )
    {
      delay(reasonableDelay);
      Serial.print(i);
      if ( rfid->PICC_ReadCardSerial() )
      {
//        Serial.print(i);
        Serial.print(";");
        for (byte b = 0; b < rfid->uid.size; b++) {
          unsigned char c = rfid->uid.uidByte[b];
          if(c<0x10) Serial.print("0");
            Serial.print(c, HEX);
        }
        rfid->PICC_HaltA();
        Serial.println();
      }
    }
//    digitalWrite(readers_sda[i], CHIP_INACTIVE);     

    delay(reasonableDelay);
  }
}


