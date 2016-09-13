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
#define EEPROM_BASE_ADDR 0

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
  SND_MSG,
  MSG_RECEIVED
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
    Heartbeat(Adafruit_NeoPixel *pixels);
    void beat();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
  private:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    Adafruit_NeoPixel *pixels;
};

typedef struct packet_header {
  uint8_t origin;
  uint8_t dest;
  uint8_t last_hop;
  uint8_t next_hop;
  uint8_t jumps;
  uint16_t seq;
};

typedef struct route {
  uint8_t gateway;
  uint8_t hops;
  uint16_t seq;
};

class MicroMesh {
  public:
    MicroMesh(RH_ASK *driver, uint8_t *rcv_buf, uint8_t buf_len, uint16_t config_addr = 0);
    void init(uint8_t id);
    void listen();
    void onPackage(void (*funct)(uint8_t *rcv_buf, uint8_t buf_len));
    uint8_t broadcast(uint8_t *snd_buf, uint8_t len);
    uint8_t send(uint8_t dst, uint8_t *snd_buf, uint8_t len);
    uint8_t getStatus();
    void setId(uint8_t id);
  private:
    uint8_t isValidPackage(packet_header pkt);
    uint16_t config_addr;
    uint8_t my_id;
    RH_ASK *driver;
    uint8_t neighbohrs[16];
    route routing_table[16];
    void (*onPackageEvt)(uint8_t *rcv_buf, uint8_t buf_len);
    uint8_t *buf;
    uint8_t buf_len;
    uint8_t enabled;
    uint16_t seq;
};

