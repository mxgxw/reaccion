#include "inttypes.h"
#include "string.h"
#include "Arduino.h"
#include "LibXBee.h"

uint8_t seq = 1;

// Received data and size
int rcvSize = 0;
byte *rcvBuff;

long lastData = 0;

// XBee initialization Status
int xBeeStatus = WAIT_MODEM;
// Reader internal status.
int readStatus = RSP_WAIT;
char buffer[BUFFSIZE];
int buffPos = 0;

void sendTo64(uint64_t address,char* data) {
  
  // TO DO: Escape characters
  uint8_t lLSB = strlen(data)+5;
  uint8_t addrMSB = (uint8_t)((address&0xFF00)>>8);
  uint8_t addrLSB = (uint8_t)(address&0xFF);
  int Sum = 0;
  uint8_t chkSum = 0;
  
  Serial.write(0x7E);
  Serial.write(0x00);
  Serial.write(lLSB);
  // Begin data
  Serial.write(0x01);
  Sum += 0x01;
  if(seq==0xFF) {
    seq = 0x01;
  } else {
    seq += 1;
  }
  Serial.write(seq);
  Sum += seq;
  Serial.write(addrMSB);
  Sum += addrMSB;
  Serial.write(addrLSB);
  Sum += addrLSB;
  Serial.write(0x00);
  Sum += 0x00;
  for(int i=0;i<strlen(data);i++){
    Serial.write(data[i]);
    Sum += data[i];
  }
  chkSum = 0xFF - (Sum&0xFF);
  Serial.write(chkSum);
}

void sendTo16(uint16_t address,char* data) {
  
  // TO DO: Escape characters
  uint8_t lLSB = strlen(data)+5;
  uint8_t addrMSB = (uint8_t)((address&0xFF00)>>8);
  uint8_t addrLSB = (uint8_t)(address&0xFF);
  int Sum = 0;
  uint8_t chkSum = 0;
  
  Serial.write(0x7E);
  Serial.write(0x00);
  Serial.write(lLSB);
  // Begin data
  Serial.write(0x01);
  Sum += 0x01;
  if(seq==0xFF) {
    seq = 0x01;
  } else {
    seq += 1;
  }
  Serial.write(seq);
  Sum += seq;
  Serial.write(addrMSB);
  Sum += addrMSB;
  Serial.write(addrLSB);
  Sum += addrLSB;
  Serial.write(0x00);
  Sum += 0x00;
  for(int i=0;i<strlen(data);i++){
    Serial.write(data[i]);
    Sum += data[i];
  }
  chkSum = 0xFF - (Sum&0xFF);
  Serial.write(chkSum);
}

/*************************************************************************
 * readResponse() listens the Serial port for WAIT_TIMEOUT milliseconds  *
 * waiting the API response from the XBee module. If the checksum of the *
 * received packet is OK, then it returns true, false otherwise.         *
 * It works as a finite automaton using the bytes readed from the Serial *
 * port as an input.                                                     *
 * You must call this function after sending an API packet to the XBee.  *
 * The function reads the response from the XBee and allocates the       *
 * memory to store the result.                                           *
 * The data received is stored in rcvBuff, the size of the received data *
 * is stored in rcvSize.                                                 *
 ************************************************************************/

bool readResponse() {

  if(rcvSize>0) {
    free(rcvBuff); // Free last buffer
    rcvSize = 0;
  }
  
  byte d; // Temporary holder for the data
  
  boolean responseFound = false; // Flag to stop reading
  
  byte lMSB,lLSB; // Packet length (MSB,LSB)
  
  int packetSize = 0; // Packet size built from the length
   
  int checkSum = 0;
  
  lastData = millis();
  while((millis()-lastData)<WAIT_TIMEOUT & !responseFound) {
    
    if(Serial.available()) {
      
      switch(readStatus) {
        case RSP_WAIT:
          d = Serial.read();
          if(d==0x7E)
            readStatus = RSP_LMSB;
          break;
        case RSP_LMSB:
          d = Serial.read();
          lMSB = d;
          readStatus = RSP_LLSB;
          break;
        case RSP_LLSB:
          d = Serial.read();
          lLSB = d;
          
          packetSize = lMSB;
          packetSize = lMSB<<8;
          packetSize = packetSize | lLSB;
          
          if(packetSize>0) {
            rcvBuff = (byte*)malloc(packetSize);
          } else {
            // Critical error, size cannot be 0
            // reset machine status to RSP_WAIT
            readStatus = RSP_WAIT;
            break;
          }
          
          readStatus = RSP_RDDATA;
          break;
        case RSP_RDDATA:
          if( rcvSize<packetSize ) { // read data
            d = Serial.read();
            rcvBuff[rcvSize++] = d;
            checkSum += d; // Calculate Checksum on the fly
          } else {
            // Last byte is the checksum
            d = Serial.read();
            checkSum += d;
            checkSum = 0xFF & checkSum;
            if(checkSum==0xFF) {
              responseFound = responseFound | true;
            } 
          }
          break;
      }
    }
  }
  
  readStatus = RSP_WAIT;
  
  if(!responseFound & (packetSize>0)) {
    // If no response was found & packetSize>0
    free(rcvBuff);
    rcvSize = 0;
  }
  
  return responseFound;
}

void switchToCommandMode() {
  xBeeStatus = COMMAND_MODE;
}

void switchToAPIMode2() {
  xBeeStatus = API_MODE2;
}


void flush_buffer() {
  for(int j=0;j<=buffPos;j++) {
    buffer[j] = 0;
  }
  buffPos = 0;
}

void append_buffer(char c) {
  if(buffPos<BUFFSIZE) {
    buffer[buffPos++] = c;
  } else {
    flush_buffer();
  }
}

bool waitFor(char *response) {
  lastData = millis();
  
  bool responseFound = false;
  
  char c;
  while((millis()-lastData)<WAIT_TIMEOUT & !responseFound) {
    while(Serial.available()) {
      c = Serial.read();
      append_buffer(c);
      if(c=='\r') {
        if(strstr(buffer,response)!=0) {
          responseFound = responseFound | true;
        }
        flush_buffer();
      }
    }
  }
  
  return responseFound;
}

bool waitFor(char *response, void (*command)()) {
  lastData = millis();
  
  char c;
  
  boolean responseFound = false;
  
  while((millis()-lastData)<WAIT_TIMEOUT & !responseFound) {
    while(Serial.available()) {
      c = Serial.read();
      append_buffer(c);
      if(c=='\r') {
        if(strstr(buffer,response)!=0) {
          responseFound = responseFound | true;
          (*command)();
        }
        flush_buffer();
      }
    }
  }
  
  return responseFound;
}

