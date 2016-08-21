/*
 * MS5637 Barometer Library for Arduino
 * Copyright 2015 - Mario Gomez <mario.gomez@teubi.co>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <Wire.h>
#include <Math.h>
#include "MS5637Barometer.h"

MS5637Barometer::MS5637Barometer() {
  this->PSENS = 0;
  this->POFFSET = 0;
  this->TCS = 0;
  this->TCO = 0;
  this->TREF = 0;
  this->TEMPSENS = 0;
  this->device_available = false;
  // Intermediate values for temperature compensation
  this->D1 = 0;
  this->D2 = 0;
  this->dT = 0;
  this->TEMP = 0;
  this->AOFF = 0;
  this->SENS = 0;
  this->TCP = 0;
  // Default oversampling value
  this->oversampling = OSR_1024;
};

/*
 * Shorcut for reading EEPROM data
 * Takes the EEPROM adddress as argument 0-7
 */
uint16_t MS5637Barometer::readEEPROM(uint8_t addr) {
  uint16_t value = 0;
  addr = 0xA0 | ((addr & 0x07)<<1);
  Wire.beginTransmission(0x76);
    Wire.send(addr);
  Wire.endTransmission();
  Wire.requestFrom(0x76,2);
  while(Wire.available()) {
    value <<=8;
    value |= Wire.read();
  }
  return value;
}

/*
 * This method starts temperature/pressure conversion and 
 * reads the result from the ADC. This method is blocking
 * and locks the execution thread until the read is finished.
 * Execution time depends of the over-sampling value.
 * (MIN .56ms MAX: 16.45ms)
 */
uint32_t MS5637Barometer::readADC(enum CONV_TYP type,enum OSR sampling) {
  uint32_t value = 0;
  uint8_t cmd = 0x40;
  
  cmd = cmd | (type<<4) | (sampling<<1);
  Wire.beginTransmission(0x76);
    Wire.send(cmd);
  
  Wire.endTransmission();
  uint16_t wait_for_read = 0;
  
  switch(sampling) {
    case OSR_256:
      wait_for_read = 560;
      break;
    case OSR_512:
      wait_for_read = 1100;
      break;
    case OSR_1024:
      wait_for_read = 2100;
      break;
    case OSR_2048:
      wait_for_read = 4200;
      break;
    case OSR_4096:
      wait_for_read = 8250;
      break;
    case OSR_8192:
      wait_for_read = 16450;
      break;
  }
  delayMicroseconds(wait_for_read);
  
  Wire.beginTransmission(0x76);
    Wire.send(0x00);
  Wire.endTransmission();
  
  Wire.requestFrom(0x76,3);
  while(Wire.available()) {
    value <<=8;
    value |= Wire.read();
  }
  return value;
}

/*
 * Initializes the sensor and reads the EEPROM data
 * for calibration.
 */
boolean MS5637Barometer::init() {
  // Reset device
  Wire.beginTransmission(0x76);
    Wire.send(0x1E);
  Wire.endTransmission();

  // Store factory default calibration values
  this->PSENS = this->readEEPROM(1);
  this->POFFSET = this->readEEPROM(2);
  this->TCS = this->readEEPROM(3);
  this->TCO = this->readEEPROM(4);
  this->TREF = this->readEEPROM(5);
  this->TEMPSENS = this->readEEPROM(6);
  this->device_available = true;

  return this->device_available;
}

/*
 * Sets the oversampling value (accuracy) for the ADC.
 * More accuracy implies longer conversion times.
 */
void MS5637Barometer::setOSR(enum OSR sampling) {
  this->oversampling = sampling;
}

int32_t MS5637Barometer::readTemperature() {
  this->D2 = this->readADC(TEMPERATURE,this->oversampling);
  Serial.print(this->D2,DEC);
  Serial.println("D2");
  this->dT = this->D2 - (this->TREF*pow(2,8));
  Serial.print(this->dT,DEC);
  Serial.println("D2");
  this->TEMP = 2000 + (this->dT * (this->TEMPSENS/pow(2,23)));
  return this->TEMP;
}

/*
 * Calculates the temperature-compensated pressure for
 * the sensor using the pre-defined accuracy.
 * OSR       Time-to-Convert(ms) Resolution(mbar) Resolution(Ã‚Â°C)
 * OSR_256          0.54            0.110            0.012
 * OSR_512          1.06            0.062            0.009
 * OSR_1024         2.08             0.039            0.006
 * OSR_2048         4.13            0.028            0.004
 * OSR_4096         8.22            0.021            0.003
 * OSR_8192        16.44            0.016            0.002
 */
int32_t MS5637Barometer::readTCPressure() {
  this->D1 = this->readADC(PRESSURE,this->oversampling);
  this->D2 = this->readADC(TEMPERATURE,this->oversampling);
  this->dT = this->D2 - (this->TREF*pow(2,8));
  this->TEMP = 2000 + (this->dT * (this->TEMPSENS/pow(2,23)));
  this->AOFF = this->POFFSET*pow(2,17) + (this->TCO*dT)/pow(2,6);
  this->SENS = this->PSENS*pow(2,16) + (this->TCS*dT)/pow(2,7);
  this->TCP = (this->D1*(this->SENS/pow(2,21)) - this->AOFF)/pow(2,15);
  return this->TCP;
}



