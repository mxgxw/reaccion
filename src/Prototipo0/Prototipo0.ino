/*************************************************************
Reaccion.net connectivity board for Arduino.
Copyright (C) 2014  Mario Gomez/mxgxw < mario.gomez _at- teubi.co >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************/
#define FILE_NAME "DATALOG.TXT"

// Global include section
#include <stdlib.h>
#include <SPI.h>
//#include <Fat16.h>
#include <SdFat.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <inttypes.h>
#include <NMEAGPS_cfg.h>
#include <NMEAGPS.h>

// Local include section
#include "Adafruit_ILI9340_stripped.h"
#include "LibXBee.h"
#include "config.h"
#include "globals.h"
#include "utils.h"

// function definitions
void xBeeFrameReceived();
void printStatus(char *msg);

void setup() {
  
  pinMode(TFT_CS,OUTPUT);
  digitalWrite(TFT_CS,HIGH);
  pinMode(TFT_DC,OUTPUT);
  digitalWrite(TFT_DC,HIGH);
  pinMode(TFT_RST,OUTPUT);
  digitalWrite(TFT_RST,HIGH);
  
  /* BEGIN IO SETUP SECTION */
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_C, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  
  //pinMode(5, OUTPUT);
  //digitalWrite(5, LOW);

  //pinMode(13,OUTPUT);
  //digitalWrite(13,HIGH);
  /* END IO SETUP SECTION */

  /* BEGIN TFT INIT SECTION */
  tftInit();
  /* END TFT INIT SECTION */

  /* BEGIN SERIAL COMM SECTION */
  //Serial.begin(9600);
  Serial1.begin(COMM_BAUDRATE);
  gpsSerial.begin(COMM_BAUDRATE);
  /* END SERIAL COMM SECTION */

  /* BEGIN XBEE SECTION */
  myXBee = new XBee900HP(&Serial1);
  myXBee->onFrameReceived(xBeeFrameReceived);
  myXBee->init(); // Blocks execution until the Xbee has been correctly initialized
  /* END XBEE SECTION */

  /* BEGIN FLASHCARD INIT */
  //const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  //char fileName[13] = FILE_BASE_NAME "00.csv";
  if (!sd.begin(SD_CS, SPI_HALF_SPEED)) {
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


void loop() {
  myXBee->listen();
  
  // Monitor inputs
  if(
      digitalRead(BTN_A)==LOW ||
      digitalRead(BTN_B)==LOW ||
      digitalRead(BTN_C)==LOW ||
      digitalRead(A2)==LOW) {
    noTone(3);
    if(olderThan(timeoutA,BTN_TIMEOUT)) {
      if(digitalRead(BTN_A)==LOW) {
        broadcastAlert(0x00,0x01);
      } else if (digitalRead(BTN_B)==LOW){
        broadcastAlert(0x00,0x02);
      } else if (digitalRead(BTN_C)==LOW) {
        broadcastAlert(0x00,0x03);
      } else if (digitalRead(A2)==LOW) {
        broadcastAlert(0x00,0x04);
      }
      timeoutA = millis();
    }
  } else {
    timeoutA = millis();
  }

  if(gpsSerial.available()) {
    gps.decode(gpsSerial.read());
    if(gps.fix().valid.location) {
      tft.fillRect(0,290, 40, 30, ILI9340_BLACK);
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
        tone(3,2000);
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
      digitalWrite(TFT_CS,HIGH);
      // LOG MSG
      if(file_ready) file.write(&myXBee->rcvBuffer[12],myXBee->rcvSize);
      if(file_ready) file.write("CH\r\n");
      if(file_ready) file.sync();
      break;
  }
}

