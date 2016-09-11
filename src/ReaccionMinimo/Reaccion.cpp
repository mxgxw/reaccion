/*************************************************************
Reaccion Minimo.
Copyright (C) 2016  Mario Gomez/mxgxw < mario.gomez _at- teubi.co >

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA  02110-1301, USA.
**************************************************************/
#include <Arduino.h>
#include "Reaccion.h"

Button::Button(uint8_t pin) {
  this->pin = pin;
  pinMode(this->pin, INPUT_PULLUP);
  this->state = digitalRead(this->pin);
  this->onRiseEvt = NULL;
  this->onFallEvt = NULL;
  this->lastChange = millis();
  this->waitDelay = 500;
}

void Button::waitForEvent() {
  uint8_t newState = this->read();
  if((millis()-this->lastChange > this->waitDelay) && newState != this->state) {
    switch(newState) {
      case 1:
        if(this->onRiseEvt)
          this->onRiseEvt(this->pin);
        break;
      case 0:
        if(this->onFallEvt)
          this->onFallEvt(this->pin);
        break;
    }
    this->state = newState;
    this->lastChange = millis();
  }
}

void Button::setFilterDelay(uint16_t delay) {
  this->waitDelay = delay;
}

void Button::onRelease(void (*funct)(uint8_t source)) {
  this->onRiseEvt = funct;
}

void Button::onPush(void (*funct)(uint8_t source)) {
  this->onFallEvt = funct;
}

uint8_t Button::read() {
  return digitalRead(this->pin);
}

Interval::Interval(uint16_t delay) {
  this->waitDelay = delay;
  this->isActive = 0;
  this->lastStart = millis();
}

void Interval::waitForEvent() {
  //Serial.println(this->isActive, DEC);
  //Serial.println(millis()-this->lastStart, DEC);
  //Serial.println(this->waitDelay, DEC);
  if(this->isActive && (millis()-this->lastStart)>this->waitDelay) {
    this->isActive = 0; // Important: This must be first because it
                        // can be overriden by the next call.
    if(this->onFinishEvt)
      this->onFinishEvt();
  }
};

void Interval::start() {
  this->isActive = 1;
  this->lastStart = millis();
  //Serial.println(this->isActive, DEC);
  //Serial.println("Starting interval");
};

void Interval::cancel() {
  this->isActive = 0;
};

void Interval::onFinish(void (*funct)()) {
   this->onFinishEvt = funct;
}

uint16_t Interval::secondsRemaining() {
  uint16_t remaining = 0;
  if(this->isActive) {
    remaining = (this->waitDelay-(millis()-this->lastStart))/1000;
  }
  return remaining;
}

Heartbeat::Heartbeat(uint8_t pin) {
  this->pin = pin;
  pinMode(this->pin, OUTPUT);
};

void Heartbeat::beat() {
  uint8_t value = 0;
  uint32_t modulo = millis()%1000;
  if(modulo<200) {
    value =  map(modulo,0,200,0,255);
  } else if (modulo>=200 && modulo<333) {
    value =  map(modulo,200,332,255,0);
  } else if (modulo>=333 && modulo<533) {
    value =  map(modulo,333,532,0,255);
  } else if (modulo>=533 && modulo<666) {
    value =  map(modulo,533,665,255,0);
  }
  analogWrite(this->pin,value);
};



