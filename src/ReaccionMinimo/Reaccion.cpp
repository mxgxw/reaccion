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
#include <Adafruit_NeoPixel.h>
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
#include "Reaccion.h"
#include <avr/eeprom.h>

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

Heartbeat::Heartbeat(Adafruit_NeoPixel *pixels) {
  this->pixels = pixels;
  //pinMode(this->pin, OUTPUT);
  this->pixels->begin();
  this->r = 50;
  this->g = 50;
  this->b = 50;
};

void Heartbeat::setColor(uint8_t r, uint8_t g, uint8_t b) {
  this->r = r;
  this->g = g;
  this->b = b;
};

void Heartbeat::beat() {
  uint8_t value = 0;
  uint32_t modulo = millis()%1000;
  if(modulo<200) {
    value =  map(modulo,0,200,0,100);
  } else if (modulo>=200 && modulo<333) {
    value =  map(modulo,200,332,100,0);
  } else if (modulo>=333 && modulo<533) {
    value =  map(modulo,333,532,0,100);
  } else if (modulo>=533 && modulo<666) {
    value =  map(modulo,533,665,100,0);
  }
  pixels->setPixelColor(0, pixels->Color(r*(value/100.0),g*(value/100.0),b*(value/100.0)));
  pixels->show();
  //analogWrite(this->pin,value);
};

MicroMesh::MicroMesh(RH_ASK *driver, uint8_t *rcv_buf, uint8_t buf_len, uint16_t config_addr) {
  this->driver = driver;
  this->buf = rcv_buf;
  this->buf_len = buf_len;
  this->config_addr = config_addr;
  for(int i=0;i<16;i++) {
    this->neighbohrs[i] = 0;
    this->routing_table[i].gateway = 0;
    this->routing_table[i].hops = 0;
    this->routing_table[i].seq = 0;
  }
}

void MicroMesh::init(uint8_t id) {
  this->my_id = id;
  this->seq = eeprom_read_word((const uint16_t *)this->config_addr);
  this->enabled = this->driver->init();
  if(this->enabled) {
    // Build packet header and broadcast
    packet_header broadcast;
    broadcast.origin = this->my_id;
    broadcast.dest = 0; // Broadcast address;
    broadcast.last_hop = this->my_id;
    broadcast.next_hop = 0;
    broadcast.jumps = 0;
    broadcast.seq = this->seq;
    this->driver->send((uint8_t *)&broadcast, sizeof(broadcast));
    this->driver->waitPacketSent();
  }
}

void MicroMesh::onPackage(void (*funct)(uint8_t *rcv_buf, uint8_t buf_len)) {
  this->onPackageEvt = funct;
}

uint8_t MicroMesh::isValidPackage(packet_header pkt) {
  return
    pkt.origin != this->my_id &&
    pkt.origin >=1 && pkt.origin <16 &&
    pkt.dest >=0 && pkt.dest < 16 &&
    pkt.origin != pkt.dest &&
    pkt.last_hop >=1 && pkt.last_hop <16 &&
    pkt.next_hop >=0 && pkt.next_hop < 16 &&
    pkt.last_hop != pkt.next_hop;
}

void MicroMesh::setId(uint8_t id) {
  this->my_id = id;
}

uint8_t MicroMesh::broadcast(uint8_t *snd_buf, uint8_t len) {
  return this->send(0, snd_buf, len);
}

uint8_t MicroMesh::send(uint8_t dst, uint8_t *snd_buf, uint8_t len) {
  if(!this->enabled) return 0; // Do not execute if not enabled
  
  uint8_t transfer_length = sizeof(packet_header)+len;
  
  if(transfer_length > this->buf_len) {
    return 0; // Data exedes buffer size
  }
  
  packet_header header;
  header.origin = this->my_id;
  header.dest = 0;
  header.last_hop = this->my_id;
  header.next_hop = 0;
  header.jumps = 0;
  header.seq = this->seq+1;

  memcpy(this->buf,(uint8_t *)&header,sizeof(header));
  memcpy(this->buf+sizeof(header),snd_buf,len);
  Serial.print("Packet size: ");
  Serial.println(transfer_length, DEC);
  this->driver->send(this->buf, transfer_length);
  this->driver->waitPacketSent();
  eeprom_write_word((uint16_t *)this->config_addr, header.seq);
  
  this->seq = header.seq;
  
  return 1;
}

uint8_t MicroMesh::getStatus() {
  return this->enabled;
}

void MicroMesh::listen() {
  if(!this->enabled)
    return; // Radio not enabled. We have failed miserably.
  
  uint8_t rcv_len = 9;
  if (this->driver->recv(this->buf, &rcv_len)) {
    Serial.println("Packet received");
    Serial.print("Length: ");
    Serial.println(rcv_len, DEC);
    if(rcv_len<sizeof(packet_header))
      return; // Packet too small. Ignore it.
    packet_header header;
    
    memcpy(&header,this->buf,sizeof(packet_header));
    Serial.print("Origin: ");
    Serial.println(header.origin, DEC);
    Serial.print("Destination: ");
    Serial.println(header.dest, DEC);
    Serial.print("Last hop: ");
    Serial.println(header.last_hop, DEC);
    Serial.print("Next hop: ");
    Serial.println(header.next_hop, DEC);
    Serial.print("Jumps: ");
    Serial.println(header.jumps, DEC);
    Serial.print("Seq: ");
    Serial.println(header.seq, DEC);
    if(!isValidPackage(header))
      return; // Invalid package.

    Serial.println("Valid Package");
    // Update routing table
    this->routing_table[header.origin].gateway = header.last_hop;
    this->routing_table[header.origin].hops = header.jumps;
    this->routing_table[header.origin].seq = header.seq;

    // Update neighborhs table
    this->neighbohrs[header.last_hop] = 1;

    if(this->routing_table[header.origin].seq <= header.seq) {
      // Pass data to App layer
      if(header.dest == this->my_id || header.dest == 0) {
        if(this->onPackageEvt && rcv_len>sizeof(packet_header))
          this->onPackageEvt(this->buf, rcv_len);
      }
      
      // Forward packet if broadcast or for another host
      if(
          header.dest != this->my_id &&
          header.jumps < 14
        ) { 
        header.last_hop = this->my_id;
        header.next_hop = this->routing_table[header.origin].gateway;
        header.jumps = header.jumps+1;
        memcpy(this->buf,(uint8_t *)&header,sizeof(header));
      }
    }
  }
}


