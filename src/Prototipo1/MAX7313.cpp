#include <Arduino.h>
#include <Wire.h>
#include "MAX7313.h"

MAX7313::MAX7313(int addr, int int_pin) {
  this->address = addr;
  this->int_pin = addr;
  attachInterrupt(digitalPinToInterrupt(int_pin), this->handle_interrupt, FALLING);
}

bool MAX7313::init() {
  bool init_ok = false;
  // On init set all pins as input
  Wire.beginTransmission(this->address);
    Wire.write(0x06);
    Wire.write(0xFF);
  init_ok &= Wire.endTransmission()==0;
  Wire.beginTransmission(this->address);
    Wire.write(0x07);
    Wire.write(0xF3);
  init_ok &= Wire.endTransmission()==0;
  return init_ok;
}

void MAX7313::handle_interrupt() {
  cli();
  Serial.println("change interrupt received");
  sei();
}

void MAX7313::digitalWrite(uint8_t pin, uint8_t val) {
  if((pin > 15) || (val > 2)) {
    return; // Return on invalid parameter value
  }
  
  // Read register state
  uint16_t output_registers = 0;
  Wire.beginTransmission(this->address);
    Wire.write(0x02);
  Wire.endTransmission();
  Wire.requestFrom(this->address,2);
    while(Wire.available()) {
      output_registers >>=8;
      output_registers |= (((uint16_t)Wire.read())<<8);
    }
    
  // Flip bits
  switch(val) {
    case 0:
      output_registers &= (((uint16_t)0xFFFF) ^ ((uint16_t)1)<<pin);
      break;
    case 1:
      output_registers |= (((uint16_t)1)<<pin);
      break;
  }
  
  // Change output
  Wire.beginTransmission(this->address);
    Wire.write(0x02);
    Wire.write(0xFF & output_registers);
    Wire.write(output_registers>>8);
  Wire.endTransmission();
}

uint8_t MAX7313::digitalRead(int pin) {
  pin &= 0xF;
  uint16_t registers = 0;
  Wire.beginTransmission(this->address);
    Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(this->address,2);
  while(Wire.available()) {
    registers >>=8;
    registers |= (((uint16_t)Wire.read())<<8);
  }
  return ((1<<pin) & registers) > 0;
}


