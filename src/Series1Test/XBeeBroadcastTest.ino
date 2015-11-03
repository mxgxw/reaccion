#include "inttypes.h"
#include "string.h"
#define BUFFSIZE 128
#define WAIT_TIMEOUT 10000

#define WAIT_MODEM 0x00
#define COMMAND_MODE 0x01
#define API_MODE2 0x02

int xBeeStatus = WAIT_MODEM;
char buffer[BUFFSIZE];
int buffPos = 0;

void setup() {
  Serial.begin(9600);
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
}

long lastData = 0;

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

boolean waitFor(char *response) {
  lastData = millis();
  
  boolean responseFound = false;
  
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

boolean waitFor(char *response, void (*command)()) {
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

// Internal status values: 
#define RSP_WAIT 0x00
#define RSP_LMSB 0x01
#define RSP_LLSB 0x02
#define RSP_RDDATA 0x03

// Received data and size
int rcvSize = 0;
byte *rcvBuff;

// Reader internal status.
int readStatus = RSP_WAIT;

boolean readResponse() {

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

#define envia Serial.write
#define esperaPor waitFor
#define luegoCambiaAModoComando switchToCommandMode
#define luegoCambiaAModoAPI2 switchToAPIMode2
#define ESPERA_MODEM WAIT_MODEM
#define MODO_COMANDO COMMAND_MODE
#define MODO_API2 API_MODE2

byte seq = 1;

void sendTo16(uint16_t address,char* data) {
  
  // TO DO: Escape characters
  byte lLSB = strlen(data)+5;
  byte addrMSB = (byte)((address&0xFF00)>>8);
  byte addrLSB = (byte)(address&0xFF);
  int Sum = 0;
  byte chkSum = 0;
  
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

void loop() {
  switch(xBeeStatus) {
    case ESPERA_MODEM:
      envia("+++");
      esperaPor("OK",luegoCambiaAModoComando);
      break;
    case MODO_COMANDO:
      envia("ATAP2\r\n");
      esperaPor("OK",luegoCambiaAModoAPI2);
      envia("ATCN\r\n");
      esperaPor("OK");
      break;
    case MODO_API2:
      digitalWrite(13, HIGH);
      
      sendTo16(0xFFFF,"Hello World!");
      
      /*
      // Ensambla el paquete
      envia(0x7E); // Frame Start
      envia(0x00); // Length MSB
      envia(0x07); // Length LSB
      envia(0x01); // Api Identifier
      envia(0x01); // Response
      envia(0xFF); // BCAST MSB
      envia(0xFF); // BCAST LSB
      envia(0x00); // Disable ACK
      envia(0x42); // Envia 'B'
      envia(0x42); // Envía 'B'
      envia(0x7B); // Checksum 7B
      */
      
      if(readResponse()) {
        digitalWrite(13, LOW);
        delay(100);
        digitalWrite(13, HIGH);
        delay(100);
        digitalWrite(13, LOW);
        delay(100);
        digitalWrite(13, HIGH);
        delay(100);
        digitalWrite(13, LOW);
        delay(100);
        digitalWrite(13, HIGH);
      } else {
        // Do nothing
      }
      break; 
  }
  delay(1000);
}
