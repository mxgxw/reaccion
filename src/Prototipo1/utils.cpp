/*************************************************************
Reaccion.net connectivity board utility functions.
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
#include "inttypes.h"
#include "utils.h"
//#include <math.h>
#include <Arduino.h>
/*
char nibbleToHex(uint8_t d) {
  if(d<10) {
    return (char)(0x30+(0x0F&d));
  } else {
    return (char)(0x41+(0x0F&(d-10)));
  }
}

char *byteToHex(uint8_t d) {
  char result[3] = "00";
  result[0] = nibbleToHex((0xF0&d)>>4);
  result[1] = nibbleToHex(0x0F&d);
  return result;
}*/
/*
float getBattVoltage() {
  return (analogRead(VBATT_PIN)*(VDD_MEASURED/1024))*2*VBATT_ADJ_FACTOR;
}
*/
bool olderThan(unsigned long time_t,unsigned long delay_t) {
  unsigned long current_t = millis();
  return (current_t-time_t)>delay_t;
}

// Returns distance in meters between two points
int32_t distance(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
  // Truncate resolution to prevent oveflows
  lat1 = lat1/1000;
  lon1 = lon1/1000;
  lat2 = lat2/1000;
  lon2 = lon2/1000;
  return ((lat2-lat1)*(lon2-lon1)+((lon2-lon1)*2)*((lon2-lon1)*2))/2308;/*
  double x = (((lon2-lon1)*PI)/1800000)*cos((((lat1+lat2)*PI)/1800000)/2);
  double y = ((lat2-lat1)*PI)/1800000;
  return (int32_t)(sqrt(x*x+y*y)*6371000);*/
}

int32_t getHWId() {
  uint32_t my_id = 0;
  my_id = SIM_UIDH^SIM_UIDMH;
  my_id = my_id^SIM_UIDML;
  my_id = my_id^SIM_UIDL;
  return my_id;
}

uint8_t getHumanId(uint32_t my_id) {
  return ((my_id>>24)&0xFF)^((my_id>>16)&0xFF)^((my_id>>8)&0xFF)^(my_id&0xFF);
}



