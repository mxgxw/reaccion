/*************************************************************
Reaccion.net connectivity board global configuration constants.
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

/*** Power & Battery ***/
// Start with 3.3, then measure the voltage
// between ground and Vdd and change it here:
#define VDD_MEASURED 2.97
// Start with 1, then calculate from dividing
// the voltage between +/- terminals and change
// it here:
#define VBATT_ADJ_FACTOR 0.766
// Change only if using an external battery with
// a different board than FIO v3.3
#define VBATT_PIN 13

// COMM Config
#define COMM_BAUDRATE 9600

// UI Button BEHAVIOUR
#define BTN_TIMEOUT 5000 // Timeout before sending message

// HARDWARE connections
// UI Button section
#define BTN_A 2
#define BTN_B 4
#define BTN_C 7
#define BTN_D A2

// TFT Connection
#define TFT_CS 9
#define TFT_DC 10
#define TFT_RST 11

// SDCARD Connection
#define SD_CS 6
//#define USE_ACMD41

// XBee buffer size
#define XBEE_BUFFER 100


