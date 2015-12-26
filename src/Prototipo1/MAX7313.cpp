#include <Arduino.h>
#include <Wire.h>
#include "MAX7313.h"

MAX7313::MAX7313(uint8_t addr, uint8_t int_pin) {
  this->address = addr;
  this->int_pin = addr;
  attachInterrupt(digitalPinToInterrupt(int_pin), this->handle_interrupt, FALLING);
}

bool MAX7313::init() {
  // On init set all pins as input
  Wire.beginTransmission(this->address);
    Wire.write(0x06);
    Wire.write(0xFF);
  uint8_t result = Wire.endTransmission();
  Wire.beginTransmission(this->address);
    Wire.write(0x07);
    Wire.write(0xFF);
  result = Wire.endTransmission();
  return true;
}

void MAX7313::handle_interrupt() {
  cli();
  Serial.println("change interrupt received");
  sei();
}

uint8_t MAX7313::digitalRead(uint8_t pin) {
  pin &= 0xF;
  uint16_t registers = 0;
  Wire.beginTransmission(this->address);
    Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(this->address,2);
  while(Wire.available()) {
    registers >>=8;
    registers |= (((uint16_t)Wire.read())<<8);
  }
  return ((1<<pin) & registers) > 0;
}

