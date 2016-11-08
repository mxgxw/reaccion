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
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <LiquidCrystal.h>
#include <SPI.h> // Not actually used but needed to compile
#include <EEPROM.h>
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
packet_header last_msg_header;
data_frame_v0 last_msg_data;
data_frame_v0 out_data_frame;
uint8_t rcv_buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t rcv_buf_len = sizeof(rcv_buf);

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
Interval interval5s(2000);
MicroMesh mesh(&driver, rcv_buf, rcv_buf_len, EEPROM_BASE_ADDR+sizeof(node_id));
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, HEARTBEAT, NEO_RGB + NEO_KHZ800);
Heartbeat heart(&pixels);
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

    // Init Neopixel
    

    // Associate interval event
    interval5s.onFinish(onFinish5s);

#ifdef DEBUG
    // Init comm ports.
    Serial.begin(9600);	  // For debug only
#endif

    // New package handler
    EEPROM.get(EEPROM_BASE_ADDR, node_id);
    mesh.onPackage(onPackage);
    
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
    if(node_id==0 || node_id>15) {
      state = CONF_ID;
      node_id=1;
      printNodeID();
    } else {
      state = BOOT;
      interval5s.start();
    }
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

void printMsg() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mensaje de ");
  lcd.setCursor(11,0);
  lcd.print(last_msg_header.origin, DEC);
  lcd.setCursor(0,1);
  switch(last_msg_data.alert_lvl) {
    case 0:
      heart.setColor(0,0,50);
      lcd.print("BLANCO");
      break;
    case 1:
      heart.setColor(50,0,0);
      lcd.print("VERDE");
      break;
    case 2:
      heart.setColor(50,50,0);
      lcd.print("AMARILLO");
      break;
    case 3:
      heart.setColor(0,50,0);
      lcd.print("ROJO");
      break;
  }
}
/***************** Event Handlers for the State Machine **************/
void onPackage(uint8_t *data, uint8_t len) {
  Serial.println("Package received");
  memcpy(&last_msg_header, data, sizeof(packet_header));

  if((len-sizeof(packet_header)) != sizeof(data_frame_v0))
    return; // Invalid payload

  memcpy(&last_msg_data, data+sizeof(packet_header), sizeof(data_frame_v0));

  switch(state) {
    case BOOT:
      break;
    case MSG_RECEIVED:
    case WAIT:
    case PRE_MSG_1:
    case PRE_MSG_2:
    case PRE_MSG_3:
    case PRE_MSG_4:
      interval5s.cancel();
      state = MSG_RECEIVED;
      printMsg();
      tone(BUZZER, 2000);
      break;
  }
}

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
      mesh.init(node_id); // Init mesh;
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
      switch(out_data_frame.alert_lvl) {
        case 0:
          //heart.setColor(0,0,50);
          break;
        case 1:
          //heart.setColor(50,0,0);
          break;
        case 2:
          //heart.setColor(50,50,0);
          break;
        case 3:
          //heart.setColor(0,50,0);
          break;
      }
      mesh.send(0,(uint8_t *)&out_data_frame, sizeof(out_data_frame));
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
          node_id = 1;
        mesh.setId(node_id);
        printNodeID();
        state = CONF_ID;
        break;
      }
    case CONF_DEC_ID:
      if(state == CONF_DEC_ID) {
        // Decrement terminal ID
        if(node_id>1)
          node_id--;
        else
          node_id = 15;
        mesh.setId(node_id);
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
          noTone(BUZZER);
          interval5s.cancel();
          //heart.setColor(50,50,50);
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
          EEPROM.put(EEPROM_BASE_ADDR, node_id);
          mesh.init(node_id); // Init mesh;
          heart.setColor(50,50,50);
          printWaitMsg();
          break;
      };
      break;
    case CONF_INC_ID:
      break;
    case CONF_DEC_ID:
      break;
    case MSG_RECEIVED:
      noTone(BUZZER);
    case WAIT:
      switch(source) {
        case BTN_1:
          state = PRE_MSG_1;
          interval5s.start();
          printGreen();
          //heart.setColor(50,50,50);
          out_data_frame.alert_lvl = 1;
        break;
        case BTN_2:
          state = PRE_MSG_2;
          interval5s.start();
          printYellow();
          //heart.setColor(50,50,50);
          out_data_frame.alert_lvl = 2;
        break;
        case BTN_3:
          state = PRE_MSG_3;
          interval5s.start();
          printWhite();
          //heart.setColor(50,50,50);
          out_data_frame.alert_lvl = 0;
        break;
        case BTN_4:
          state = PRE_MSG_4;
          interval5s.start();
          printRed();
          //heart.setColor(50,50,50);
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
  mesh.listen();
}

