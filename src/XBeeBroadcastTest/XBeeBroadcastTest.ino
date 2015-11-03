#include "inttypes.h"
#include "string.h"
#include "LibXBee.h"
#include "drylib_esp.h"

void setup() {
  Serial.begin(9600);
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
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
