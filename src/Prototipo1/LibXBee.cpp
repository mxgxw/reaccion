/*************************************************************
Reaccion.net connectivity board XBee event-based abstraction layer.
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
#include <stdlib.h>
#include <Arduino.h>
#include "inttypes.h"

#include "LibXBee.h"

XBee900HP::XBee900HP(HardwareSerial *serial) {
  this->seq = 0;
  this->rcvSize = 0;
  this->rcvBuffer = NULL;

  this->xBeeStatus = WAIT_MODEM;
  this->buffer = (char*)malloc(BUFFSIZE);
  this->buffPos = 0;

  this->d = '\0';

  this->responseFound = false;

  this->lMSB = 0;
  this->lLSB = 0;

  this->packetSize = 0;
  this->checkSum = 0;

  this->lastData = 0;
  this->lastSerialData = 0;

  this->readStatus = RSP_WAIT;

  this->frameReceivedHandler = NULL;
  
  this->_HardSerial = serial;
  
  this->escapeNext = false;
}

bool XBee900HP::init() {
  bool waitForInit = true;
  
  bool confirmATAP2 = false;
  bool confirmATCN = false;
  
  while(waitForInit) {
    switch(this->xBeeStatus) {
      case WAIT_MODEM:
        this->_HardSerial->write("+++");
        if(this->waitFor("OK")) {
          this->xBeeStatus = COMMAND_MODE;
        }
        break;
      case COMMAND_MODE:
      
        this->_HardSerial->write("ATAP2\r\n");
        confirmATAP2 = this->waitFor("OK");
        this->_HardSerial->write("ATCN\r\n");
        confirmATCN = this->waitFor("OK");
        
        if(confirmATAP2 && confirmATCN) {
          waitForInit = false;
          this->xBeeStatus = API_MODE2;
        } else {
          this->xBeeStatus = WAIT_MODEM;
        }
        break;
    }
  }
  
  return (this->xBeeStatus==API_MODE2);
}

void XBee900HP::sendTo64RAW(uint32_t addr_high, uint32_t addr_low, uint8_t* data, uint16_t dSize) {
  
  // Process Frames only in API Mode 2
  if(this->xBeeStatus!=API_MODE2) {
    return;
  }
  
  uint16_t mSize = dSize+14;
  
  uint32_t Sum = 0;
  byte chkSum = 0;
  
  uint8_t tmpData = 0;
  
  // Header:
  this->_HardSerial->write(0x7E);
  tmpData = (mSize & 0xFF00)>>8;
  this->escapeAndWrite(tmpData);
  tmpData = mSize & 0xFF;
  this->escapeAndWrite(tmpData);
  
  // Frame
  tmpData = 0x10;
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  
  
  if(this->seq==0xFF) {
    this->seq = 0x01;
  } else {
    this->seq += 1;
  }
  this->escapeAndWrite(seq);
  Sum += seq;
  
  // Address HIGH
  tmpData = (uint8_t)((addr_high & 0xFF000000)>>24);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)((addr_high & 0x00FF0000)>>16);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)((addr_high & 0x0000FF00)>>8);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)(addr_high & 0x000000FF);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  
  
  // Address LOW
  tmpData = (uint8_t)((addr_low & 0xFF000000)>>24);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)((addr_low & 0x00FF0000)>>16);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)((addr_low & 0x0000FF00)>>8);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = (uint8_t)(addr_low & 0x000000FF);
  this->escapeAndWrite(tmpData);
  Sum += tmpData;

  tmpData = 0xFF;
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  tmpData = 0xFE;
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  
  tmpData = 0x00;
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  
  tmpData = 0x30;
  this->escapeAndWrite(tmpData);
  Sum += tmpData;
  
  for(int i=0;i<dSize;i++){
    tmpData = data[i];
    this->escapeAndWrite(tmpData);
    Sum += tmpData;
  }
  
  chkSum = 0xFF - (Sum&0xFF);
  this->escapeAndWrite(chkSum);
}

void XBee900HP::sendTo64(uint32_t addr_high, uint32_t addr_low, char* data) {
  sendTo64RAW(addr_high,addr_low,(uint8_t*)data,strlen(data));
}

void XBee900HP::listen() {
  if(this->_HardSerial->available() && (this->xBeeStatus==API_MODE2)) {
    this->d = this->_HardSerial->read();
    
    if(this->d==0x7D) {
      this->escapeNext = true;
      return;
    }
   
    if(this->escapeNext) {
      this->d = this->d^0x20;
      this->escapeNext = false;
    }
    
    switch(this->readStatus) {
      case RSP_WAIT:
        if(this->d==0x7E) {
          this->readStatus = RSP_LMSB;
          this->checkSum = 0; // rESET CHECKSUM
          if(this->rcvSize>0) {
            free(this->rcvBuffer); // Free last buffer
            this->rcvSize = 0;
          }
        }
        break;
      case RSP_LMSB:
        this->lMSB = this->d;
        this->readStatus = RSP_LLSB;
        break;
      case RSP_LLSB:
        this->lLSB = this->d;
        
        this->packetSize = this->lMSB;
        this->packetSize = this->lMSB<<8;
        this->packetSize = this->packetSize | this->lLSB;
        
        if(this->packetSize>0) {
          this->rcvBuffer = (byte*)malloc(this->packetSize);
        } else {
          // Critical error, size cannot be 0
          // reset machine status to RSP_WAIT
          this->readStatus = RSP_WAIT;
          break;
        }
        
        this->readStatus = RSP_RDDATA;
        break;
      case RSP_RDDATA:
        if( this->rcvSize<this->packetSize ) { // read data
          this->rcvBuffer[this->rcvSize++] = this->d;
          this->checkSum += this->d; // Calculate Checksum on the fly
        } else {
          // Last byte is the checksum
          this->checkSum += this->d;
          this->checkSum = 0xFF & this->checkSum;
          if(this->checkSum==0xFF) {
            if(this->frameReceivedHandler!=NULL) {
              this->frameReceivedHandler();
            }
          }
          this->readStatus = RSP_WAIT; 
        }
        break;
    }
  }
}

void XBee900HP::onFrameReceived(void (*handler)()) {
  this->frameReceivedHandler = handler;
}

bool XBee900HP::waitFor(const char *response, void (*command)()) {
  this->lastSerialData = millis();
  
  char c;
  
  boolean responseFound = false;
  
  while((millis()-this->lastSerialData)<WAIT_TIMEOUT && !responseFound) {
    while(this->_HardSerial->available()) {
      c = this->_HardSerial->read();
      this->append_buffer(c);
      if(c=='\r') {
        if(strstr(this->buffer,response)!=0) {
          responseFound = responseFound | true;
          (*command)();
        }
        this->flush_buffer();
      }
    }
  }
  
  return responseFound;
}

bool XBee900HP::waitFor(const char *response) {
  this->lastSerialData = millis();
  
  boolean responseFound = false;
  
  char c;
  while((millis()-this->lastSerialData)<WAIT_TIMEOUT && !responseFound) {
    while(this->_HardSerial->available()) {
      c = this->_HardSerial->read();
      this->append_buffer(c);
      if(c=='\r') {
        if(strstr(this->buffer,response)!=0) {
          responseFound = responseFound | true;
        }
        this->flush_buffer();
      }
    }
  }
  
  return responseFound;
}

void XBee900HP::flush_buffer() {
  for(int j=0;j<=this->buffPos;j++) {
    this->buffer[j] = 0;
  }
  this->buffPos = 0;
}

void XBee900HP::append_buffer(char c) {
  if(this->buffPos<BUFFSIZE) {
    this->buffer[this->buffPos++] = c;
  } else {
    flush_buffer();
  }
}

void XBee900HP::escapeAndWrite(uint8_t &data) {
  if(
    data == 0x7E ||
    data == 0x7D ||
    data == 0x11 ||
    data == 0x13
    ) {
    this->_HardSerial->write(0x7D);
    this->_HardSerial->write(data^0x20);
  } else {
    this->_HardSerial->write(data);
  }
}

