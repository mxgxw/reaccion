#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
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
#include "data_frames.h"
#include "globals.h"
#include "utils.h"
#define MBEDTLS_SHA1_C
#include "sha1.h"
#include "sha1.c"

#define _F(string_literal) (reinterpret_cast<__FlashStringHelper *>(PSTR(string_literal)))

#define FILE_NAME "DATALOG.TXT"

SoftwareSerial Serial4(24, 25);
/* Aliases for serial devices */
#define USB_SER Serial
#define XBEE_SER Serial1
#define GSM_SER Serial2
#define GPS_SER Serial3
#define BT_SER Serial4

NMEAGPS gps;

gps_fix fix_data;

Adafruit_ILI9340 tft = Adafruit_ILI9340(SPI1_CS, DISPLAY_DC, SPI1_RST);
MS5637Barometer baroSensor;
MAX7313 externalIO(0x20, IO_INT0);

mbedtls_sha1_context sha1_context;

uint8_t shared_key[SHARED_KEY_SIZE+1];

uint32_t msgCount = 0;

uint32_t getMsgCount() {
  uint32_t count = 0;
  count = EEPROM.read(EEPROM_MSG_COUNT_OFFSET);
  count |= EEPROM.read(EEPROM_MSG_COUNT_OFFSET+1)<<8;
  count |= EEPROM.read(EEPROM_MSG_COUNT_OFFSET+2)<<16;
  return count;
}

uint32_t incMsgCount() {
  msgCount += 1;
  EEPROM.write(EEPROM_MSG_COUNT_OFFSET, msgCount & 0xFF);
  EEPROM.write(EEPROM_MSG_COUNT_OFFSET, (msgCount>>8) & 0xFF);
  EEPROM.write(EEPROM_MSG_COUNT_OFFSET, (msgCount>>16) & 0xFF);
  return getMsgCount();
}

void setSharedKey(uint8_t *key) {
  for(int i=0;i<SHARED_KEY_SIZE;i++) {
    EEPROM.write(EEPROM_SHARED_KEY_OFFSET+i,key[i]);
  }
}

void getSharedKey(uint8_t *shared) {
  for(int i=0;i<SHARED_KEY_SIZE;i++) {
    shared[i] = EEPROM.read(EEPROM_SHARED_KEY_OFFSET+i);
  }
} 

void setup() {
  /* READ NON-VOLATILE MEMORY */
  // Get Hardware-ID
  MY_ID = getHWId();
  // Get Shared KEY from EEPROM
  getSharedKey(shared_key);
  // Get message count from EEPROM
  msgCount = getMsgCount();
  
  /* SETUP IO PINS */
  // Set up pins for XBee Module
  pinMode(XBEE_RESET, OUTPUT); // XBee reset pin
  digitalWrite(XBEE_RESET, HIGH);
  pinMode(XBEE_SLEEP, OUTPUT); // XBee sleep pin
  digitalWrite(XBEE_SLEEP, HIGH);
  
  /* IO PINS FOR GSM MODULE */
  pinMode(SIM900_PWR, OUTPUT); // Power pin
  digitalWrite(SIM900_PWR, LOW);
  pinMode(SIM900_RESET, OUTPUT); // Reset pin
  digitalWrite(SIM900_RESET, LOW);
  pinMode(SIM900_RI, INPUT); // Ring line
  pinMode(SIM900_STATUS, INPUT); // Status pin

  /* IO PINS FOR GPS MODULE */
  pinMode(GPS_ONOFF, OUTPUT); // GPS start pulse
  digitalWrite(GPS_ONOFF, LOW);
  pinMode(GPS_TM, INPUT); // 1PPS Time mark
  pinMode(GPS_WU, INPUT); // Wake up signal
  pinMode(GPS_RST, OUTPUT);
  digitalWrite(GPS_RST, HIGH); // GPS Reset

  /* IO PINS FOR BLUETOOTH MODULE */
  pinMode(BT_EN, OUTPUT); // Enable power on BT
  digitalWrite(BT_EN, LOW);
  pinMode(BT_CMD, OUTPUT); // Command mode BT
  digitalWrite(BT_EN, LOW);

  /* IO PINS DISPLAY & SD MODULE */
  pinMode(DISPLAY_DC, OUTPUT); // TFT Data/Command
  digitalWrite(DISPLAY_DC, LOW);
  pinMode(DISPLAY_SDCS, OUTPUT); // SD Card Chip select
  digitalWrite(DISPLAY_SDCS, HIGH);
  pinMode(SPI1_CS, OUTPUT); // TFT Chip select
  digitalWrite(SPI1_CS, HIGH);
  pinMode(DISPLAY_BL, OUTPUT); // TFT & buttons backlight
  digitalWrite(DISPLAY_BL, LOW);

  /* OTHER HARDWARE IO PINS */
  pinMode(EN_12V, OUTPUT); // 12V boost converter startup
  digitalWrite(EN_12V, LOW); // Disable on boot.
  pinMode(BUZZER_ACT, OUTPUT); // Buzzer activation
  digitalWrite(BUZZER_ACT, LOW); // Disable on boot.
  pinMode(DISPLAY_BL, OUTPUT); //Backlight control

  /* IO PINS FOR GSM MODULE */
  // TODO: Implement event based user interface
  pinMode(IO_INT0, INPUT_PULLUP); // External interrupt pin
  attachInterrupt(IO_INT0, intIoExpansor, CHANGE);

  /* SETUP LOW LEVEL DEVICES */
  // Enable USB debug serial port
  USB_SER.begin(115200);
  // Enable serial port for XBee Module
  XBEE_SER.begin(9600);
  // Enable serial port for SIM900 Module
  GSM_SER.begin(9600);
  // Enable serial port for GPS
  GPS_SER.begin(4800);

#ifdef DEBUG_REACCION
  USB_SER.println("COM ports started.");
  USB_SER.print("Core ID: ");
  USB_SER.println(MY_ID, HEX);
  USB_SER.print("Network Key: ");
  USB_SER.println(shared_key);
#endif
  // Enable AltSoft serial port for Bluetooth
  BT_SER.begin(38400);

#ifdef DEBUG_REACCION
  USB_SER.println("Software COM port for Bluetooth enabled.");
#endif

  // Enable SPI bus for TFT Display and SD Card
  SPI.setMOSI(SPI1_MOSI);
  SPI.setMISO(SPI1_MISO);
  SPI.setSCK(SPI1_SCK);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);

#ifdef DEBUG_REACCION
  USB_SER.println("SPI Port enabled.");
#endif
  // Enable I2C bus for other devices
  Wire.begin();
  
#ifdef DEBUG_REACCION
  USB_SER.println("I2C Port enabled.");
#endif
  /* SETUP HARDWARE-ABSTRACTED DEVICES  */

  /* BEGIN TFT INIT SECTION */
#ifdef DEBUG_REACCION
  USB_SER.println("Starting TFT");
#endif
  // Turn on backlight
  analogWrite(DISPLAY_BL, 255);
#ifdef DEBUG_REACCION
  USB_SER.println("Backlight ON");
#endif
  tftInit();
#ifdef DEBUG_REACCION
  USB_SER.println("TFT started");
#endif

  // Baro/Temp sensor I2C setup
  baroSensor.init();
  baroSensor.setOSR(OSR_2048);
#ifdef DEBUG_REACCION
  USB_SER.println("Barometric sensor started");
#endif

  // Init defaults for MAX7313
  externalIO.init();
  externalIO.digitalWrite(10, 0);
  externalIO.digitalWrite(11, 0);
#ifdef DEBUG_REACCION
  USB_SER.println("External IO enabled");
#endif
  
  /* BEGIN XBEE SECTION */
#ifdef DEBUG_REACCION
  USB_SER.println("Starting XBee radio.");
#endif
  printStatus(("Iniciando Radio..."), ILI9340_YELLOW);
  myXBee = new XBee900HP(&Serial1);
  myXBee->onFrameReceived(xBeeFrameReceived);
  myXBee->init(); // Blocks execution until the Xbee has been correctly initialized
  printStatus(("Radio iniciado."), ILI9340_GREEN);
#ifdef DEBUG_REACCION
  USB_SER.println("Radio succesfully started");
#endif
  /* END XBEE SECTION */
  delay(100); // Delay needed to see messages
  /* BEGIN GPS INIT SECTION*/
#ifdef DEBUG_REACCION
  USB_SER.println("Starting GPS");
#endif
  printStatus(("Iniciando GPS..."), ILI9340_YELLOW);
  initGPS();
  printStatus(("GPS Iniciado."), ILI9340_GREEN);
#ifdef DEBUG_REACCION
  USB_SER.println("GPS Started");
#endif
  /* END GPS INIT SECTION */
  delay(100); // Delay needed to see messages
  /* BEGIN FLASHCARD INIT */
  //const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  //char fileName[13] = FILE_BASE_NAME "00.csv";
#ifdef DEBUG_REACCION
  USB_SER.println("Trying to access SD Card");
#endif
  if (!sd.begin(DISPLAY_SDCS, SPI_HALF_SPEED)) {
    printStatus(("Error Iniciando SD"), ILI9340_RED);
#ifdef DEBUG_REACCION
  USB_SER.println("Error on SD Card init");
#endif
  }
  /*
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      printStatus(("CNNOTCREAT"));
    }
  }*/
  delay(100);
#ifdef DEBUG_REACCION
  USB_SER.println("Creating file in SD");
#endif
  if (!file.open(FILE_NAME, O_CREAT | O_WRITE)) {
    printStatus(("Error Iniciando FS"), ILI9340_RED);
#ifdef DEBUG_REACCION
  USB_SER.println("Error creating file");
#endif
  } else {
#ifdef DEBUG_REACCION
  USB_SER.println("SD Memory ready");
#endif
    printStatus(("MEM LISTA"), ILI9340_GREEN);
    file.println(("Starting up..."));
    file.sync();
    file_ready = true;
  }
  /* END FLASHCARD INIT */

  /* BEGIN INIT UI globals */
  timeoutA = millis();
  /* END INIT UI globals */
#ifdef DEBUG_REACCION
  USB_SER.println("[[REACCION READY]]");
#endif
  printStatus(("REACCION listo."), ILI9340_GREEN);
  delay(100);

  // Init SHA-1
  mbedtls_sha1_init(&sha1_context);
}

void intIoExpansor() {
  // ISR for changes on IO Expansor board.
}

void selfMonitor() {
  tft.fillRect(250,0, 320, 20, ILI9340_BLACK);
  tft.setCursor(250,0);
  /* Not implemented on this board revision
  switch(externalIO.digitalRead(CHG_STAT)) {
    case 0:
      tft.print('-');
      break;
    case 1:
      tft.print('C');
      break;
  }
  switch(externalIO.digitalRead(PWR_STAT)) {
    case 0:
      tft.print('-');
      break;
    case 1:
      tft.print('P');
      break;
  } */
  switch(gps.fix().valid.location) {
    case 0:
      tft.setTextColor(ILI9340_RED);
      break;
    case 1:
      tft.setTextColor(ILI9340_GREEN);
      break;
  }
  tft.print('G');
  tft.setTextColor(ILI9340_WHITE);
  int battVoltage = analogRead(A10);
  float fVoltage = (3.3/1024)*battVoltage*2;
  tft.print(fVoltage);
}

int currTest = 0;

void printWait(int seconds) {
  for(int i=0;i<seconds;i++) {
#ifdef DEBUG_REACCION
    USB_SER.print(seconds-i,DEC);
    USB_SER.print("\n");
#endif
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

void controlBackLight() {
  if((millis()-blTimeout)<MAX_BL || buzzer_enabled == true) {
    externalIO.digitalWrite(10, 0);
    externalIO.digitalWrite(11, 0);
    analogWrite(DISPLAY_BL, 255);
  } else {
    externalIO.digitalWrite(10, 1);
    externalIO.digitalWrite(11, 1);
    analogWrite(DISPLAY_BL, 0);
  }
}

struct Message {
  uint32_t origin = -1;
  int level = -1;
  long lastMessage = 0;
  uint32_t lat = 0;
  uint32_t lon = 0;
};

struct Message lastMessages[4];

void pushQueue(struct Message msg) {
  lastMessages[3].origin = lastMessages[2].origin;
  lastMessages[3].level = lastMessages[2].level;
  lastMessages[3].lastMessage = lastMessages[2].lastMessage;
  lastMessages[3].lat = lastMessages[2].lat;
  lastMessages[3].lon = lastMessages[2].lon;
  lastMessages[2].origin = lastMessages[1].origin;
  lastMessages[2].level = lastMessages[1].level;
  lastMessages[2].lastMessage = lastMessages[1].lastMessage;
  lastMessages[2].lat = lastMessages[1].lat;
  lastMessages[2].lon = lastMessages[1].lon;
  lastMessages[1].origin = msg.origin;
  lastMessages[1].level = msg.level;
  lastMessages[1].lastMessage = msg.lastMessage;
  lastMessages[1].lat = msg.lat;
  lastMessages[1].lon = msg.lon;
}

void drawQueue() {
  tft.fillRect(0,100, 320, 140, ILI9340_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 100);
  drawMsg(1,1);
  drawMsg(2,1);
  drawMsg(3,1);
}

void drawMsg(int idx, int fmt) {
  switch(lastMessages[idx].level) {
    case 1:
      tft.setTextColor(ILI9340_GREEN);
      tft.print((" K"));
      tft.print(getHumanId(lastMessages[idx].origin));
      tft.print((": "));
      tft.print(("[Verde] ("));
      break;
    case 2:
      tft.setTextColor(ILI9340_YELLOW);
      tft.print((" K"));
      tft.print(getHumanId(lastMessages[idx].origin));
      tft.print((": "));
      tft.print(("[Amarillo] ("));
      break;
    case 3:
      tft.setTextColor(ILI9340_RED);
      tft.print((" K"));
      tft.print(getHumanId(lastMessages[idx].origin));
      tft.print((": "));
      tft.print(("[Rojo] ("));
      break;
    case 4:
      tft.setTextColor(ILI9340_WHITE);
      tft.print((" K"));
      tft.print(getHumanId(lastMessages[idx].origin));
      tft.print((": "));
      tft.print(("[Blanco] ("));
      break;
    default:
      tft.setTextColor(ILI9340_WHITE);
      tft.println(("  [Sin estado]"));
      break;
  }
  if(lastMessages[idx].level>=1 && lastMessages[idx].level<=4) {
      tft.print((millis()-lastMessages[idx].lastMessage)/60000, DEC);
      tft.println((" min)"));
      tft.print((" "));
      switch(fmt) {
        case 0:
          tft.print((millis()-lastMessages[idx].lat)/100000);
          tft.print(("N -"));
          tft.print((millis()-lastMessages[idx].lat)/100000);
          tft.println(("W"));
          break;
        case 1:
          tft.print(("@ "));
          tft.print(distance(gps.fix().lat,gps.fix().lon,lastMessages[idx].lat,lastMessages[idx].lon),DEC);
          tft.println("m");
          break;
      }
  }
}

void drawLastStatus() {
  tft.fillRect(0,40, 320, 40, ILI9340_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 40);
  drawMsg(0,0);
}

long lastUpdate = 0;

void loop() {
  myXBee->listen();
  controlBackLight();
  
  if((millis()-lastUpdate)>10000) {
    drawLastStatus();
    drawQueue();
    selfMonitor();
    lastUpdate = millis();
  }
  // Monitor inputs
  if(
      externalIO.digitalRead(BTN_A)==LOW ||
      externalIO.digitalRead(BTN_B)==LOW ||
      externalIO.digitalRead(BTN_C)==LOW ||
      externalIO.digitalRead(BTN_D)==LOW) {
    tft.setCursor(75, 224);
    if(externalIO.digitalRead(BTN_A)==LOW) {
      tft.fillRect(0,220, 320, 20, ILI9340_GREEN);
      tft.setTextColor(ILI9340_BLACK);
    } else if(externalIO.digitalRead(BTN_B)==LOW) {
      tft.fillRect(0,220, 320, 20, ILI9340_YELLOW);
      tft.setTextColor(ILI9340_BLACK);
    } else if(externalIO.digitalRead(BTN_C)==LOW) {
      tft.fillRect(0,220, 320, 20, ILI9340_RED);
      tft.setTextColor(ILI9340_WHITE);
    } else if(externalIO.digitalRead(BTN_D)==LOW) {
      tft.fillRect(0,220, 320, 20, ILI9340_WHITE);
      tft.setTextColor(ILI9340_BLACK);
    }
    tft.print(("[Enviando en "));
    tft.print((5000-(millis()-timeoutA))/1000, DEC);
    tft.print(("]"));
    //USB_SERIAL.println("Button down");
    blTimeout = millis();
    if(olderThan(timeoutA,BTN_TIMEOUT)) {
#ifdef DEBUG_REACCION
      USB_SERIAL.println("Sending message");
#endif
      if(externalIO.digitalRead(BTN_A)==LOW) {
        broadcastAlert(0,0x01);
      } else if (externalIO.digitalRead(BTN_B)==LOW){
        broadcastAlert(0,0x02);
      } else if (externalIO.digitalRead(BTN_C)==LOW) {
        broadcastAlert(0,0x03);
      } else if (externalIO.digitalRead(BTN_D)==LOW) {
        broadcastAlert(0,0x04);
      }
      timeoutA = millis();
    }
    if(buzzer_enabled) {
      digitalWrite(BUZZER_ACT, LOW);
      delay(100);
      digitalWrite(EN_12V, LOW); // Disable 12V power source
      buzzer_enabled = false;
    }
  } else {
    tft.fillRect(0,220, 320, 20, ILI9340_BLACK);
    timeoutA = millis();
  }

  if(GPS_SER.available()) {
    uint8_t c = GPS_SER.read();
    gps.decode(c);
    if(gps.fix().valid.location) {

#ifdef DEBUG_REACCION
      USB_SERIAL.println("GPS Fixed");
#endif
      /*
      tft.fillRect(0,290, 40, 50, ILI9340_RED);
      tft.setTextColor(ILI9340_WHITE);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print(("Te encuentras a \n"));
      tft.print(distance(gps.fix().lat,gps.fix().lon,139277359,-898317289),DEC);
      tft.print(("m de Getsemani"));
      */
    }
  }
  // Clear status message if older than X seconds
  if(olderThan(lastStatus,5000)) {
    tft.fillRect(0,200, 320, 40, ILI9340_BLACK);
  }
}

void initGPS() {
  GPS_SER.setTimeout(10000);
  if(GPS_SER.find("GPGGA")) {
#ifdef DEBUG_REACCION
    USB_SERIAL.println("GPS Data found");
#endif
  } else {
#ifdef DEBUG_REACCION
    USB_SERIAL.println("NF _Trying to start GPS Unit");
#endif
    digitalWrite(GPS_ONOFF,HIGH);
    delay(500);
    digitalWrite(GPS_ONOFF,LOW);
  }
}

void tftInit() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9340_RED);
  tft.setTextSize(2);
  tft.print(("REACCION KIT-"));
  tft.print(getHumanId(MY_ID), DEC);
  tft.setCursor(0, 20);
  tft.setTextColor(ILI9340_WHITE);
  tft.setTextSize(2);
  tft.println(("Tu estado:"));
  drawLastStatus();
  tft.setCursor(0, 80);
  tft.println(("Mensajes recientes:"));
  drawQueue();
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
#if PROTOCOL == PROTOCOL_0
  appendUINT16(PROTOCOL_0); // Protocol version
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
#ifdef DEBUG_REACCION
  USB_SERIAL.println("Sending message");
#endif
#endif
#if PROTOCOL == PROTOCOL_1
/*
typedef struct rx_frame_protocol_1{
  uint16_t version = 1;
  uint32_t origin;
  uint8_t category;
  uint8_t level;
  uint32_t lon;
  uint32_t lat;
  uint8_t month;
  uint8_t date;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  int32_t temp;
  int32_t pressure;
  uint32_t running;
};
 */
  rx_frame_protocol_1 tx_data;
  tx_data.origin = MY_ID;
  tx_data.category = 0x00;
  tx_data.level = level;
  tx_data.lon = gps.fix().lon;
  tx_data.lat = gps.fix().lat;
  tx_data.month = gps.fix().dateTime.month;
  tx_data.date = gps.fix().dateTime.date;
  tx_data.hours = gps.fix().dateTime.hours;
  tx_data.minutes = gps.fix().dateTime.minutes;
  tx_data.seconds = gps.fix().dateTime.seconds;
  tx_data.temp = baroSensor.readTemperature();
  tx_data.pressure = baroSensor.readTCPressure();
  tx_data.running = msgCount;
  Serial.println(sizeof(uint16_t),DEC);
  Serial.println(sizeof(tx_data),DEC);
  incMsgCount(); // Incrementar conteo de mensajes
#ifdef DEBUG_REACCION
  USB_SERIAL.print("Message lenght: ");
  USB_SERIAL.println(sizeof(tx_data),DEC);
#endif

  memcpy(xBeeData,&tx_data,sizeof(tx_data));
  mbedtls_sha1_starts(&sha1_context);
  mbedtls_sha1_update(&sha1_context,(uint8_t*)&tx_data,sizeof(tx_data));
  mbedtls_sha1_update(&sha1_context,(uint8_t*)&shared_key,sizeof(SHARED_KEY_SIZE));
  mbedtls_sha1_finish(&sha1_context,xBeeData+sizeof(tx_data));
  mbedtls_sha1_free(&sha1_context);
  myXBee->sendTo64RAW(0x00000000,0x0000FFFF,xBeeData,sizeof(tx_data)+20);
#endif
  
  lastMessages[0].level = level;
  lastMessages[0].origin = getHumanId(MY_ID);
  lastMessages[0].lon = gps.fix().lon;
  lastMessages[0].lat = gps.fix().lat;
  lastMessages[0].lastMessage = millis();
  drawLastStatus();
}


void printStatus(const char *msg, uint8_t txtColor) {
  tft.fillRect(0,200, 320, 40, txtColor);
  tft.setCursor(0,200);
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

int32_t extractINT16(uint8_t *start) {
  return *(uint16_t*)(start);
}

int32_t extractINT32(uint8_t *start) {
  return *(uint32_t*)(start);
}

uint8_t equalSHA1(uint8_t a[20], uint8_t b[20]) {
  uint8_t valid = 1;
  for(int i=0;i<20;i++) {
    if(!(valid &= (a[i]==b[i])))
      break;
  }
  return valid;
}

void xBeeFrameReceived() {
  if(myXBee->rcvSize==0) {
    return;
  }
  uint16_t proto_version = 0;
  switch(myXBee->rcvBuffer[0]) {
    //case 0x8B:
      //USB_SERIAL.print("Frame id: ");
      //USB_SERIAL.print(myXBee->rcvBuffer[1],HEX);
      //USB_SERIAL.print("\n");
      //USB_SERIAL.print("Delivery Status: ");
      /*
      switch(myXBee->rcvBuffer[5]) {
        case 0x00:
          //USB_SERIAL.print("Sucess\n");
          tft.setTextColor(ILI9340_GREEN);
          printStatus(("MSG Enviado"));
          break;
        case 0x01:
          //USB_SERIAL.print("MAC ACK Failure\n");
          break;
        case 0x21:
          ////USB_SERIAL.print("Network ACK Failure\n");
          tft.setTextColor(ILI9340_RED);
          printStatus(("ERROR RED"));
          break;
        case 0x25:
          //USB_SERIAL.print("Route not Found\n");
          break;
        case 0x74:
          //USB_SERIAL.print("Payload too large\n");
          break;
        case 0x75:
          //USB_SERIAL.print("Indirect message unrequested\n");
          break;
        }*/
    //  break;
    case 0x90: // Data received
#ifdef DEBUG_REACCION
      USB_SERIAL.print("Package received: ");
      USB_SERIAL.println(myXBee->rcvSize, DEC);
#endif
      //tft.setCursor(0, 60);
      //tft.fillRect(0,60, tft.width()-1, 100, ILI9340_BLACK);
      
      //tft.setTextSize(3);
      //tft.print(myXBee->rcvSize,DEC);
      if(myXBee->rcvSize>=12) {
        proto_version = extractINT16(&myXBee->rcvBuffer[RX_FRAME_OFFSET]);
#ifdef DEBUG_REACCION
        USB_SERIAL.print("PROTO_");
        USB_SERIAL.println(proto_version, HEX);
#endif
        switch(proto_version) {
          case 0xFFFF: // BEGIN PROTOCOL 0xFFFF PARSING
            break;
          case 0x00: // BEGIN PROTOCOL 0 PARSING
            if(myXBee->rcvSize==29) {
              digitalWrite(EN_12V, HIGH); // Enable 12V power source
              delay(100); // Wait for power supply to estabilize itself
              digitalWrite(BUZZER_ACT, HIGH);
              buzzer_enabled = true;
              switch(myXBee->rcvBuffer[RX_FRAME_OFFSET+3]) {
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
              uint32_t lon = extractINT32(&myXBee->rcvBuffer[RX_FRAME_OFFSET+4]);
              uint32_t lat = extractINT32(&myXBee->rcvBuffer[RX_FRAME_OFFSET+8]);
              struct Message msg;
              msg.origin = myXBee->rcvBuffer[RX_FRAME_OFFSET+2];
              msg.level = myXBee->rcvBuffer[RX_FRAME_OFFSET+3];
              msg.lat = lat;
              msg.lon = lon;
              msg.lastMessage = millis();
              pushQueue(msg);
              drawQueue();
            }
            break; // END PROTOCOL 0 PARSING
          case 0x01: // BEGIN PROTOCOL 1 PARSING
#ifdef DEBUG_REACCION
            USB_SERIAL.println("Handling protocol 1 package");
#endif
            if(myXBee->rcvSize==(RX_FRAME_OFFSET+sizeof(rx_frame_protocol_1)+20)) {
#ifdef DEBUG_REACCION
              USB_SERIAL.println("Handling protocol 1 package");
              for(int i=RX_FRAME_OFFSET;i<myXBee->rcvSize;i++) {
                USB_SERIAL.print(myXBee->rcvBuffer[i], HEX);
              }
              USB_SERIAL.println("--END");
#endif
              digitalWrite(EN_12V, HIGH); // Enable 12V power source
              delay(100); // Wait for power supply to estabilize itself
              digitalWrite(BUZZER_ACT, HIGH);
              buzzer_enabled = true;

              rx_frame_protocol_1 rcv_data;
              memcpy(&rcv_data,myXBee->rcvBuffer+RX_FRAME_OFFSET,sizeof(rx_frame_protocol_1));
              
              struct Message msg;
              msg.origin = rcv_data.origin;
#ifdef DEBUG_REACCION
              USB_SERIAL.print("Origin");
              USB_SERIAL.print(rcv_data.origin,HEX);
#endif
              msg.level = rcv_data.level;
#ifdef DEBUG_REACCION
              USB_SERIAL.print("Lvl");
              USB_SERIAL.print(rcv_data.level,DEC);
#endif
              msg.lat = rcv_data.lat;
              msg.lon = rcv_data.lon;
#ifdef DEBUG_REACCION
              USB_SERIAL.print("Lat");
              USB_SERIAL.print(rcv_data.lat,HEX);
              USB_SERIAL.print("Lon");
              USB_SERIAL.print(rcv_data.lon,HEX);
#endif

#ifdef DEBUG_REACCION
              uint8_t *proc_data = (uint8_t *)&rcv_data;
              for(int i=0;i<28;i++) {
                USB_SERIAL.print(proc_data[i], HEX);
              }
              USB_SERIAL.println("END--");
#endif
              msg.lastMessage = millis();
              pushQueue(msg);
              drawQueue();
            }
            break; // END PROTOCOL 1 PARSING
          case 0xFF: // TODO: BEGIN CHANGE KEY CONFIGURATION

            break; // END CHANGE SHARED KEY
          default:
            printStatus(("Protocol not supported"), ILI9340_RED);
            break;
        }
        // Store packet even if protocol is not known.
        digitalWrite(SPI1_CS,HIGH);
        if(file_ready) file.write(&myXBee->rcvBuffer[12],myXBee->rcvSize);
        if(file_ready) file.write("CH\r\n");
        if(file_ready) file.sync();
      }
      break;
  }
}

