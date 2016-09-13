// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver;


typedef struct packet_header {
  uint8_t origin;
  uint8_t dest;
  uint8_t last_hop;
  uint8_t next_hop;
  uint8_t jumps;
  uint16_t seq;
};

typedef struct data_frame_v0 {
  uint8_t frame_version = 0;
  uint8_t alert_lvl;
};

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t rcv_buf_len = sizeof(rcv_buf);

packet_header packet;
data_frame_v0 alert;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
    if(millis()%10000 == 0) {
        packet.origin = 15;
        packet.dest = 0;
        packet.last_hop = 15;
        packet.next_hop = 0;
        packet.jumps = 0;
        packet.seq = packet.seq+1;
        alert.alert_lvl = 0;
        memcpy(buf, (uint8_t *)&packet, sizeof(packet));
        memcpy(buf+sizeof(packet), (uint8_t *)&alert, sizeof(alert));
        driver.send((uint8_t *)buf, strlen(msg));
        driver.waitPacketSent();
    }
}
