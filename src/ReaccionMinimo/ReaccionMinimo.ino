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
#include <RH_ASK.h>
#include <LiquidCrystal.h>
#include <SPI.h> // Not actually used but needed to compile
#include "Reaccion.h"
#include "dataframes.h"

#define DEBUG // Comment out to prevent debug messages on serial
/********************** BEGIN NET CONFIG **************************/
#define MAX_HOPS 15
/************************ END NEt CONFIG **************************/
/********************** BEGIN IO MAPPING **************************/
// LCD screen IO mapping
#define LCD_RS 8
#define LCD_E  9
#define LCD_D4 2
#define LCD_D5 3
#define LCD_D6 4
#define LCD_D7 6
#define LCD_BL 10 // Pin 10 supports PWM on Atmega328p
// IO Buttons
#define BTN_1 A0
#define BTN_2 A1
#define BTN_3 A2
#define BTN_4 A3
// RF IO
#define RF_RX 11
#define RF_TX 12
#define RF_PTT 0
// Buzzer
#define BUZZER 7
// Battery level
#define BATT_LVL A7
// User interface
#define HEARTBEAT 5
/************************ END IO MAPPING **************************/
/************************ BEGIN GLOBALS ***************************/
PROGRAM_STATE state = BOOT;
uint8_t node_id = 0;
data_frame_v0 out_data_frame;
/************************** END GLOBALS ***************************/
/************************ BEGIN DRIVERS ***************************/
// Basic network conectivity
RH_ASK driver(2000, RF_RX, RF_TX, RF_PTT, false);
// Pheripherals
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Button btn1(BTN_1);
Button btn2(BTN_2);
Button btn3(BTN_3);
Button btn4(BTN_4);
// Helpers
Interval interval5s(5000);
Heartbeat heart(HEARTBEAT);
/************************** END DRIVERS ***************************/

/************************** BEGIN CODE  ***************************/
void setup()
{
    pinMode(13, INPUT);
    pinMode(LCD_BL, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    
    // Associate button events
    btn1.onRelease(onRelease);
    btn2.onRelease(onRelease);
    btn3.onRelease(onRelease);
    btn4.onRelease(onRelease);

    btn1.onPush(onPush);
    btn2.onPush(onPush);
    btn3.onPush(onPush);
    btn4.onPush(onPush);

    // Associate interval event
    interval5s.onFinish(onFinish5s);

#ifdef DEBUG
    // Init comm ports.
    Serial.begin(9600);	  // For debug only
#endif

    // Init radio transceiver
    if (!driver.init())
#ifdef DEBUG
         Serial.println("init failed");
#endif

    // Init user interface
    digitalWrite(LCD_BL, HIGH);
    lcd.begin(16, 2); // User interface
    printBootMsg();
    
#ifdef DEBUG
    Serial.println("Boot message");
#endif
    interval5s.start();
    tone(BUZZER, 2000, 500);
}

/************************** LCD Draw Helpers ************************/
void printBootMsg() {
  lcd.setCursor(0, 0);
  lcd.print("Reaccion Minimo");
  lcd.setCursor(0,1);
  lcd.print("Iniciando...");
}

void printWaitMsg() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Esperando");
  lcd.setCursor(0, 1);
  lcd.print("Mensajes");
}

void printNodeID() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ID de nodo:");
  lcd.setCursor(0, 1);
  lcd.print(node_id, DEC);
}

void printGreen() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("VERDE");
  lcd.setCursor(0,1);
  lcd.print("Envia en ");
}

void printYellow() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("AMARILLO");
  lcd.setCursor(0,1);
  lcd.print("Envia en ");
}

void printWhite() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BLANCO");
  lcd.setCursor(0,1);
  lcd.print("Envia en ");
}

void printRed() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ROJO");
  lcd.setCursor(0,1);
  lcd.print("Envia en ");
}

void printSentMsg() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mensaje");
  lcd.setCursor(0,1);
  lcd.print("enviado.");
}

void printSeconds() {
  lcd.setCursor(9,1);
  lcd.print(interval5s.secondsRemaining(), DEC);
}

/***************** Event Handlers for the State Machine **************/
void onFinish5s() {
  switch(state) {
    case BOOT:
#ifdef DEBUG
      Serial.println("Jump to conf detect.");
#endif
      state = CONF_DETECT;
      lcd.setCursor(0,1);
      lcd.print("[4] -> Config");
      interval5s.start();
      break;
    case CONF_DETECT:
#ifdef DEBUG
      Serial.println("Jump to home.");
#endif
      state = WAIT;
      printWaitMsg();
      break;
    case CONF_ID:
#ifdef DEBUG
      Serial.println("On conf id.");
#endif
      break;
    case CONF_INC_ID:
      break;
    case CONF_DEC_ID:
      break;
    case WAIT:
      printWaitMsg();
      break;
    case PRE_MSG_1:
    case PRE_MSG_2:
    case PRE_MSG_3:
    case PRE_MSG_4:
#ifdef DEBUG
      Serial.println("Enviando Mensaje");
#endif
      state = SND_MSG;
    case SND_MSG:
      // Send actual message
      state = WAIT;
      driver.send((uint8_t *)&out_data_frame, sizeof(out_data_frame));
      driver.waitPacketSent();
      printSentMsg();
      interval5s.start();
      break;
  };
}

void onRelease(uint8_t source) {
#ifdef DEBUG
  Serial.print("Release evt ");
  Serial.println(source, DEC);
#endif
  switch(state) {
    case BOOT:
      break;
    case CONF_DETECT:
      switch(source) {
        case BTN_4:
          tone(BUZZER, 500, 50);
#ifdef DEBUG
          Serial.println("Jump to id config.");
#endif
          state = CONF_ID;
          printNodeID();
          break;
      };
      break;
    case CONF_ID:
      switch(source) {
        case BTN_1:
          state = CONF_INC_ID;
          tone(BUZZER, 500, 50);
          break;
        case BTN_2:
          state = CONF_DEC_ID;
          tone(BUZZER, 500, 50);
          break;
      }; // Do not break. Automatically jump to the next cases
    case CONF_INC_ID:
      if(state == CONF_INC_ID) {
        // Increment terminal ID
        if(node_id<15)
          node_id++;
        else
          node_id = 0;
        printNodeID();
        state = CONF_ID;
        break;
      }
    case CONF_DEC_ID:
      if(state == CONF_DEC_ID) {
        // Decrement terminal ID
        if(node_id>0)
          node_id--;
        else
          node_id = 15;
        printNodeID();
        state = CONF_ID;
      }
      break;
    case WAIT:
      break;
    case PRE_MSG_1:
    case PRE_MSG_2:
    case PRE_MSG_3:
    case PRE_MSG_4:
      switch(source) {
        case BTN_1:
        case BTN_2:
        case BTN_3:
        case BTN_4:
          state = WAIT;
          interval5s.cancel();
          printWaitMsg();
          break;
      };
      break;
    case SND_MSG:
      break;
  };
}

void onPush(uint8_t source) {
#ifdef DEBUG
  Serial.print("Push evt ");
  Serial.println(source, DEC);
#endif
  switch(state) {
    case BOOT:
      break;
    case CONF_DETECT:
      break;
    case CONF_ID:
      switch(source) {
        case BTN_3:
          state = WAIT;
          printWaitMsg();
          break;
      };
      break;
    case CONF_INC_ID:
      break;
    case CONF_DEC_ID:
      break;
    case WAIT:
      switch(source) {
        case BTN_1:
          state = PRE_MSG_1;
          interval5s.start();
          printGreen();
          out_data_frame.alert_lvl = 1;
        break;
        case BTN_2:
          state = PRE_MSG_2;
          interval5s.start();
          printYellow();
          out_data_frame.alert_lvl = 2;
        break;
        case BTN_3:
          state = PRE_MSG_3;
          interval5s.start();
          printWhite();
          out_data_frame.alert_lvl = 0;
        break;
        case BTN_4:
          state = PRE_MSG_4;
          interval5s.start();
          printRed();
          out_data_frame.alert_lvl = 3;
        break;
      };
      break;
    case PRE_MSG_1:
      break;
    case PRE_MSG_2:
      break;
    case PRE_MSG_3:
      break;
    case PRE_MSG_4:
      break;
    case SND_MSG:
      break;
  };
}

void mainLoopStateMachine() {
  switch(state) {
    case BOOT:
      break;
    case CONF_DETECT:
      break;
    case CONF_ID:
      break;
    case CONF_INC_ID:
      break;
    case CONF_DEC_ID:
      break;
    case WAIT:
      break;
    case PRE_MSG_1:
    case PRE_MSG_2:
    case PRE_MSG_3:
    case PRE_MSG_4:
      printSeconds();
      break;
    case SND_MSG:
      break;
  };
}

/***************** Cooperative multi-tasking ;) ******************/
void loop()
{
  interval5s.waitForEvent();
  btn1.waitForEvent();
  btn2.waitForEvent();
  btn3.waitForEvent();
  btn4.waitForEvent();
  mainLoopStateMachine();
  heart.beat();
}

