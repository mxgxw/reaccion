/*************************************************************
Reaccion.net connectivity board global variables.
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
// Current battery level
float bat_lvl = 0;

// Uses NeoGPS for NMEA parsing
SoftwareSerial gpsSerial(8,A5);
static NMEAGPS gps;

// Custom non-blocking xBee library
XBee900HP * myXBee;
uint8_t xBeeData[XBEE_BUFFER];
char xBeeBuffPos = 0;

// Filesystem libs
SdFat sd;
// Log file.
SdFile file;
bool file_ready = false;

// TFT Screen
Adafruit_ILI9340_stripped tft = Adafruit_ILI9340_stripped(TFT_CS, TFT_DC, TFT_RST);

// UI variables
long timeoutA = 0;
long timeoutB = 0;
long timeoutC = 0;
long timeoutD = 0;
long lastStatus = 0;
static gps_fix rmc_data;
long last_batt_read = 0;
