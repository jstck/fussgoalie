#include <MFRC522.h>
#include <SPI.h>


//RFID chips are active low
#define CHIP_ACTIVE LOW
#define CHIP_INACTIVE HIGH

#define INTERRUPT_MODE RISING

//Chip select pins of the different RFID reader devices
#define NUM_READERS 4
const int readers_sda[NUM_READERS] = { 46, 47, 48, 49};



//Pins and interrupts for the sensors
const int laser_pins[4] = {18, 19, 20, 21};
const int laser_irq[4] = {2, 3, 4, 5};

const int resetPin = 44; //Actual reset pin
//const int irqPin = 45; //Not used

const int reasonableDelay = 25;
const int resetDelay = 100;

//Nice global interrupt-unsafe array to store when stuff happens.
int laser_events[4] = {0, 0, 0, 0};

int mode=0;

#define MODE_REG 1
#define MODE_PLAY 2

//rfid objects per reader
MFRC522 * rfids[NUM_READERS];


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

  Serial.println("INFO;SPI");
  SPI.begin();

  //Force a reset of all readers.
  Serial.println("INFO;Reset");
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, LOW);
  delay(resetDelay);
  digitalWrite(resetPin, HIGH);
  delay(resetDelay);


  //Create RFID reader objects
  for(int i=0; i<NUM_READERS; i++)
  {
    Serial.print("INFO;RFID create");
    Serial.println(i);
    rfids[i] = new MFRC522(readers_sda[i], resetPin);
    delay(reasonableDelay);
  }
  DisableAllRFID(true); //Disable stuff again in case we're with the broken MFRC522 library
  
  //Initiate all readers
  for(int i=0; i<NUM_READERS; i++)
  {
    Serial.print("INFO;RFID init ");
    Serial.println(i);
    rfids[i]->PCD_Init();
    delay(reasonableDelay);
  }
  DisableAllRFID();
  
  
  Serial.println("INFO;Calibrating lasers");
  for(int i=0; i<4; i++)
  {
    pinMode(laser_pins[i], INPUT);
  }
  attachInterrupt(laser_irq[0], irq0, INTERRUPT_MODE);
  attachInterrupt(laser_irq[1], irq1, INTERRUPT_MODE);
  attachInterrupt(laser_irq[2], irq2, INTERRUPT_MODE);
  attachInterrupt(laser_irq[3], irq3, INTERRUPT_MODE);
  
  mode = MODE_REG;
  
  Serial.println("INFO;Ready");
}

//Interrupt handlers for signalling goal detector times. Totally safe.
void irq0()
{
  laser_events[0] = micros();
}
void irq1()
{
  laser_events[1] = micros();
}
void irq2()
{
  laser_events[2] = micros();
}
void irq3()
{
  laser_events[3] = micros();
}



void loop()
{  
  switch(mode)
  {
    case MODE_REG:
      main_reg();
      break;
    case MODE_PLAY:
      main_play();
      break;
    default:
      //We are in some screwed up unknown mode. Revert to play mode
      mode = MODE_PLAY; 
  }
  
  //Check for input on serial to change mode
  int c = Serial.read();
  while(c>=0)
  {
    switch(c)
    {
      case 'R':
      case 'r':
        mode = MODE_REG;
        Serial.println("INFO;MODE_REG");
        break;
      case 'P':
      case 'p':
        mode = MODE_PLAY;
        for(int i=0; i<4; i++) laser_events[i]=0; //Clear events when going into play mode
        Serial.println("INFO;MODE_PLAY");
        break;
    }
    c = Serial.read();
  }
  
  delay(reasonableDelay);
}


void main_reg()
{
  MFRC522 *rfid;
    
  for(int i=0; i<NUM_READERS; i++)
  {
    rfid=rfids[i];

    delay(reasonableDelay);
    
    if( rfid->PICC_IsNewCardPresent() )
    {
      delay(reasonableDelay);
      if ( rfid->PICC_ReadCardSerial() )
      {
        Serial.print("R");
        Serial.print(i);
        Serial.print(";");
        for (byte b = 0; b < rfid->uid.size; b++) {
          unsigned char c = rfid->uid.uidByte[b];
          if(c<0x10) Serial.print("0");
            Serial.print(c, HEX);
        }
        rfid->PICC_HaltA(); //Disable card until it is removed and read again
        Serial.println();
      }
    }

    delay(reasonableDelay);
  }
}

void main_play()
{
  for(int i=0; i<4; i++)
  {
    if(laser_events[i]>0)
    {
      long time = laser_events[i];
      laser_events[i]=0;
      
      Serial.print("G;");
      Serial.print(i);
      Serial.print(";");
      Serial.println(time);
    }
  }
}
