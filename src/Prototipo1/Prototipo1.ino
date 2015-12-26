#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
//#include <SD.h>
#include <SdFat.h>
#include "pinmap.h"
#include "NMEAGPS_cfg.h"
#include "NMEAGPS.h"
#include "MS5637Barometer.h"
#include "MAX7313.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
#include "LibXBee.h"
#include "globals.h"
#include "utils.h"

#define FILE_NAME "DATALOG.TXT"

SoftwareSerial Serial4(24, 25);
/* Aliases for serial devices */
#define USB_SER Serial
#define XBEE_SER Serial1
#define GSM_SER Serial2
#define GPS_SER Serial3
#define BT_SER Serial4

static NMEAGPS gps;

static gps_fix fix_data;

static int currentVoltage = 0;

void stuur(byte adres, byte reg, byte data){  // small hint: stuur = dutch for send
   Wire.beginTransmission(adres);
     Wire.send( reg);
     Wire.send( data);
    Wire.endTransmission();
}

void baroReset() {
  Wire.beginTransmission(0xEC);
    Wire.send(0x1E);
  Wire.endTransmission();
}

Adafruit_ILI9340 tft = Adafruit_ILI9340(SPI1_CS, DISPLAY_DC, SPI1_RST);
MS5637Barometer baroSensor;
MAX7313 externalIO(0x20, IO_INT0);

void setup() {
  /* SETUP IO PINS */
  // Set up pins for XBee Module
  pinMode(XBEE_RESET, OUTPUT); // XBee reset pin
  digitalWrite(XBEE_RESET, HIGH);
  pinMode(XBEE_SLEEP, OUTPUT); // XBee sleep pin
  digitalWrite(XBEE_SLEEP, HIGH);
  
  // Set up pins for GSM module
  pinMode(SIM900_PWR, OUTPUT); // Power pin
  digitalWrite(SIM900_PWR, LOW);
  pinMode(SIM900_RESET, OUTPUT); // Reset pin
  digitalWrite(SIM900_RESET, LOW);
  pinMode(SIM900_RI, INPUT); // Ring line
  pinMode(SIM900_STATUS, INPUT); // Status pin

  // Set up pins for GPS Module
  pinMode(GPS_ONOFF, OUTPUT); // GPS start pulse
  digitalWrite(GPS_ONOFF, LOW);
  pinMode(GPS_TM, INPUT); // 1PPS Time mark
  pinMode(GPS_WU, INPUT); // Wake up signal
  pinMode(GPS_RST, OUTPUT);
  digitalWrite(GPS_RST, HIGH); // GPS Reset

  // Set up pins for Bluetooth Module
  pinMode(BT_EN, OUTPUT); // Enable power on BT
  digitalWrite(BT_EN, LOW);
  pinMode(BT_CMD, OUTPUT); // Command mode BT
  digitalWrite(BT_EN, LOW);

  // Set up pins for TFT screen and card reader
  pinMode(DISPLAY_DC, OUTPUT); // TFT Data/Command
  digitalWrite(DISPLAY_DC, LOW);
  pinMode(DISPLAY_SDCS, OUTPUT); // SD Card Chip select
  digitalWrite(DISPLAY_SDCS, HIGH);
  pinMode(SPI1_CS, OUTPUT); // TFT Chip select
  digitalWrite(SPI1_CS, HIGH);
  pinMode(DISPLAY_BL, OUTPUT); // TFT & buttons backlight
  digitalWrite(DISPLAY_BL, LOW);

  // Set up other IO Pins
  pinMode(EN_12V, OUTPUT); // 12V boost converter startup
  digitalWrite(EN_12V, LOW); // Disable on boot.
  pinMode(BUZZER_ACT, OUTPUT); // Buzzer activation
  digitalWrite(BUZZER_ACT, LOW); // Disable on boot.
  pinMode(DISPLAY_BL, OUTPUT); //Backlight control

  pinMode(IO_INT0, INPUT_PULLUP); // External interrupt pin
  attachInterrupt(IO_INT0, intIoExpansor, CHANGE);

  /* SETUP LOW LEVEL DEVICES */
  // Enable USB debug serial port
  USB_SER.begin(115200);
  // Enable serial port for XBee Module
  XBEE_SER.begin(9600);
  // Enable serial port for SIM900 Module
  //GSM_SER.begin(9600);
  // Enable serial port for GPS
  GPS_SER.begin(4800);

  // Enable AltSoft serial port for Bluetooth
  BT_SER.begin(38400);

  // Enable SPI bus for TFT Display and SD Card
  SPI.setMOSI(SPI1_MOSI);
  SPI.setMISO(SPI1_MISO);
  SPI.setSCK(SPI1_SCK);
  SPI.begin();

  // Enable I2C bus for other devices
  Wire.begin();
  
  /* SETUP HARDWARE-ABSTRACTED DEVICES  */

  // Turn on backlight
  analogWrite(DISPLAY_BL, 50);
  tft.begin();

  Serial.println("Starting barometric sensor");
  // Baro/Temp sensor I2C setup
  baroSensor.init();
  baroSensor.setOSR(OSR_2048);
  externalIO.init();
  
  /* BEGIN XBEE SECTION */
  myXBee = new XBee900HP(&Serial1);
  myXBee->onFrameReceived(xBeeFrameReceived);
  myXBee->init(); // Blocks execution until the Xbee has been correctly initialized
  /* END XBEE SECTION */

  /* BEGIN TFT INIT SECTION */
  tftInit();
  initGPS();
  /* END TFT INIT SECTION */

  /* BEGIN FLASHCARD INIT */
  //const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  //char fileName[13] = FILE_BASE_NAME "00.csv";
  if (!sd.begin(DISPLAY_SDCS, SPI_HALF_SPEED)) {
    printStatus(F("ERR->"));
  }
  /*
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      printStatus(F("CNNOTCREAT"));
    }
  }*/
  if (!file.open(FILE_NAME, O_CREAT | O_WRITE)) {
    printStatus(F("CNNTOPEN"));
  } else {
    printStatus(F("MEM LISTA"));
    file.println(F("Starting up..."));
    file.sync();
    file_ready = true;
  }
  /* END FLASHCARD INIT */

  /* BEGIN INIT UI globals */
  //bat_lvl = getBattVoltage();
  timeoutA = millis();
  //timeoutB = millis();
  //timeoutC = millis();
  //timeoutD = millis();
  /* END INIT UI globals */
}

void intIoExpansor() {
  // ISR for changes on IO Expansor board.
}

void selfMonitor() {
  // Check battery voltaje level & estimated lifetime.
}

int currTest = 0;

void printWait(int seconds) {
  for(int i=0;i<seconds;i++) {
    Serial.print(seconds-i,DEC);
    Serial.print("\n");
    delay(1000);
  }
}

void sim900powerON() {
  digitalWrite(SIM900_PWR,HIGH);
  delay(1500);
  digitalWrite(SIM900_PWR,LOW);
  delay(3000);
}

void sim900powerOFF() {
  digitalWrite(SIM900_PWR,HIGH);
  delay(2500);
  digitalWrite(SIM900_PWR,LOW);
  delay(3000);
}

void loop() {
  myXBee->listen();
  
  // Monitor inputs
  if(
      externalIO.digitalRead(BTN_A)==LOW ||
      externalIO.digitalRead(BTN_B)==LOW ||
      externalIO.digitalRead(BTN_C)==LOW ||
      externalIO.digitalRead(BTN_D)==LOW) {
    Serial.println("Button down");
    if(olderThan(timeoutA,BTN_TIMEOUT)) {
      Serial.println("Sending message");
      if(digitalRead(BTN_A)==LOW) {
        broadcastAlert(0x00,0x01);
      } else if (digitalRead(BTN_B)==LOW){
        broadcastAlert(0x00,0x02);
      } else if (digitalRead(BTN_C)==LOW) {
        broadcastAlert(0x00,0x03);
      } else if (digitalRead(BTN_D)==LOW) {
        broadcastAlert(0x00,0x04);
      }
      timeoutA = millis();
    }
    digitalWrite(BUZZER_ACT, LOW);
    delay(100);
    digitalWrite(EN_12V, LOW); // Enable 12V power source
  } else {
    timeoutA = millis();
  }

  if(GPS_SER.available()) {
    uint8_t c = GPS_SER.read();
    gps.decode(c);
    if(gps.fix().valid.location) {
      Serial.println("GPS Fixed");
      tft.fillRect(0,290, 40, 50, ILI9340_BLACK);
      tft.setTextColor(ILI9340_WHITE);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print(F("Te encuentras a \n"));
      tft.print(distance(gps.fix().lat,gps.fix().lon,139277359,-898317289),DEC);
      tft.print(F("m de Getsemani"));
    }
  }
  // Clear status message if older than X seconds
  if(olderThan(lastStatus,5000)) {
    tft.fillRect(0,290, 240, 30, ILI9340_BLACK);
  }
}

void initGPS() {
  GPS_SER.setTimeout(10000);
  if(GPS_SER.find("GPGGA")) {
    Serial.println("GPS Data found");
  } else {
    Serial.println("NF _Trying to start GPS Unit");
    digitalWrite(GPS_ONOFF,HIGH);
    delay(500);
    digitalWrite(GPS_ONOFF,LOW);
  }
}

void tftInit() {
  tft.begin();
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9340_RED);
  tft.setTextSize(3);
  tft.print(F("Reaccion.net"));
  tft.setCursor(0, 30);
  tft.setTextColor(ILI9340_WHITE);
  tft.setTextSize(2);
  tft.println(F("Ultimo mensaje:"));
  tft.setCursor(0, 180);
  tft.println(F("Mi posicion:"));
}


void appendUINT8(int8_t v) {
  memcpy(xBeeData+xBeeBuffPos,&v,sizeof(v));
  xBeeBuffPos += sizeof(v);
}

void appendUINT16(uint16_t v) {
  memcpy(xBeeData+xBeeBuffPos,&v,sizeof(v));
  xBeeBuffPos += sizeof(v);
}

void appendUINT32(int32_t v) {
  memcpy(xBeeData+xBeeBuffPos,&v,sizeof(v));
  xBeeBuffPos += sizeof(v);
}

void broadcastAlert(uint8_t category,uint8_t level) {
  clearData();
  appendUINT16(0x00); // Protocol version
  appendUINT8(category); // Send Category CAT 00 = TEST
  appendUINT8(level); // Send Message
  appendUINT32(gps.fix().lon); // Send Longitude
  appendUINT32(gps.fix().lat); // Send Latitude
  appendUINT8(gps.fix().dateTime.month);
  appendUINT8(gps.fix().dateTime.date);
  appendUINT8(gps.fix().dateTime.hours);
  appendUINT8(gps.fix().dateTime.minutes);
  appendUINT8(gps.fix().dateTime.seconds);
  myXBee->sendTo64RAW(0x00000000,0x0000FFFF,xBeeData,xBeeBuffPos);
  printStatus(F("Trying to send"));
  Serial.println("Trying to send");
}


void printStatus(const __FlashStringHelper *msg) {
  tft.fillRect(0,290, 240, 30, ILI9340_BLACK);
  tft.setCursor(0,290);
  tft.setTextSize(2);
  tft.print(msg);
  lastStatus = millis();
}

void clearData() {
  for(int i=0;i<XBEE_BUFFER;i++) {
    xBeeData[i] = 0;
  }
  xBeeBuffPos = 0;
}

int32_t extractINT32(uint8_t *start) {
  return *(uint32_t*)(start);
}

void xBeeFrameReceived() {
  if(myXBee->rcvSize==0) {
    return;
  }
  switch(myXBee->rcvBuffer[0]) {
    //case 0x8B:
      //Serial.print("Frame id: ");
      //Serial.print(myXBee->rcvBuffer[1],HEX);
      //Serial.print("\n");
      //Serial.print("Delivery Status: ");
      /*
      switch(myXBee->rcvBuffer[5]) {
        case 0x00:
          //Serial.print("Sucess\n");
          tft.setTextColor(ILI9340_GREEN);
          printStatus(F("MSG Enviado"));
          break;
        case 0x01:
          //Serial.print("MAC ACK Failure\n");
          break;
        case 0x21:
          ////Serial.print("Network ACK Failure\n");
          tft.setTextColor(ILI9340_RED);
          printStatus(F("ERROR RED"));
          break;
        case 0x25:
          //Serial.print("Route not Found\n");
          break;
        case 0x74:
          //Serial.print("Payload too large\n");
          break;
        case 0x75:
          //Serial.print("Indirect message unrequested\n");
          break;
        }*/
    //  break;
    case 0x90: // Data received 
      
      tft.setCursor(0, 60);
      tft.fillRect(0,60, tft.width()-1, 100, ILI9340_BLACK);
      
      tft.setTextSize(3);
      if((char)myXBee->rcvBuffer[14]==0x00) {
        digitalWrite(EN_12V, HIGH); // Enable 12V power source
        delay(100); // Wait for power supply to estabilize itself
        digitalWrite(BUZZER_ACT, HIGH);
        switch(myXBee->rcvBuffer[15]) {
          case 0x01:
            tft.setTextColor(ILI9340_GREEN);
            break;
          case 0x02:
            tft.setTextColor(ILI9340_YELLOW);
            break;
          case 0x03:
            tft.setTextColor(ILI9340_RED);
            break;
          case 0x04:
            tft.setTextColor(ILI9340_WHITE);
            break;
        }
      }
      if(myXBee->rcvBuffer[8]==0x20) {
        tft.print(F("CHONTE CANTA  "));      
      } else if(myXBee->rcvBuffer[8]==0x1F) {
        tft.print(F("CHIO CANTA    "));      
      } else if(myXBee->rcvBuffer[8]==0x1A) {
        tft.print(F("CHEJE CANTA   "));      
      } else {
        tft.print(F("OTRO CANTA    "));      
      }
      switch(myXBee->rcvBuffer[15]) {
        case 0x01:
          tft.print(F("VERDE\n"));
          break;
        case 0x02:
          tft.print(F("AMARILLO\n"));
          break;
        case 0x03:
          tft.print(F("ROJO\n"));
          break;
        case 0x04:
          tft.print(F("GRIS\n"));
          break;
      }
      tft.setTextSize(2);
      uint32_t lon = extractINT32(&myXBee->rcvBuffer[16]);
      uint32_t lat = extractINT32(&myXBee->rcvBuffer[20]);
      tft.print(F("ESTA A "));
      tft.print(distance(lat,lon,gps.fix().lat,gps.fix().lon),DEC);
      tft.print(F("m DE TI Y A "));
      tft.print(distance(lat,lon,139277359,-898317289),DEC);
      tft.print(F("m DE CM GETSEMANI.\n"));
      digitalWrite(SPI1_CS,HIGH);
      // LOG MSG
      if(file_ready) file.write(&myXBee->rcvBuffer[12],myXBee->rcvSize);
      if(file_ready) file.write("CH\r\n");
      if(file_ready) file.sync();
      break;
  }
}

