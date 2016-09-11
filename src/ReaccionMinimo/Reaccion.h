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
enum PROGRAM_STATE {
  BOOT = 0,
  CONF_DETECT,
  CONF_ID,
  CONF_INC_ID,
  CONF_DEC_ID,
  WAIT,
  PRE_MSG_1,
  PRE_MSG_2,
  PRE_MSG_3,
  PRE_MSG_4,
  SND_MSG
};

class Button {
  public:
    Button(uint8_t pin);
    void waitForEvent();
    void setFilterDelay(uint16_t delay);
    void onRelease(void (*funct)(uint8_t source));
    void onPush(void (*funct)(uint8_t source));
    uint8_t read();
  private:
    uint8_t pin;
    uint8_t state;
    uint16_t waitDelay;
    void (*onRiseEvt)(uint8_t source);
    void (*onFallEvt)(uint8_t source);
    uint32_t lastChange;
};

class Interval {
  public:
    Interval(uint16_t delay);
    void waitForEvent();
    void setInterval(uint16_t delay);
    void start();
    void cancel();
    void onFinish(void (*funct)());
    uint16_t secondsRemaining();
  private:
    uint8_t isActive;
    uint16_t waitDelay;
    uint32_t lastStart;
    void (*onFinishEvt)();
};

class Heartbeat {
  public:
    Heartbeat(uint8_t pin);
    void beat();
  private:
    uint8_t pin;
};

