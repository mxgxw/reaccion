/*
  LibXBee.h - Library for using XBee API mode.
  Created by Mario Gomez, February 12, 2014.
  Source code released under the GNU/GPL v2.0 license.
*/
#ifndef LibXBee_h
#define LibXBee_h

#define BUFFSIZE 128
#define WAIT_TIMEOUT 10000

// XBee Internal status
#define WAIT_MODEM 0x00
#define COMMAND_MODE 0x01
#define API_MODE2 0x02

// Internal status values: 
#define RSP_WAIT 0x00
#define RSP_LMSB 0x01
#define RSP_LLSB 0x02
#define RSP_RDDATA 0x03

void sendTo16(uint16_t address,char* data);
bool readResponse();
void switchToCommandMode();
void switchToAPIMode2();

void flush_buffer();
void append_buffer(char c);

bool waitFor(char *response);
bool waitFor(char *response, void (*command)());

#endif
