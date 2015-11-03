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

/*** Power & Battery **/
// Gets the current battery voltage.
// Note: Voltage calculated depends on constants
// defined in globals.h
//float getBattVoltage();

/*** conversions and strings **/
/*
// Converts the lower 4 bits to hexadecimal representation
char nibbleToHex(uint8_t d);
// Converts a byte to its hexadecimal representation
char *byteToHex(uint8_t d);
*/
bool olderThan(unsigned long time_t,unsigned long delay_t);

int32_t distance(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);
