/*************************************************************
Reaccion.net connectivity board XBee event-based abstraction layer.
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
#define BUFFSIZE 128
#define WAIT_TIMEOUT 10000

#define WAIT_MODEM 0x00
#define COMMAND_MODE 0x01
#define API_MODE2 0x02

// Internal status values: 
#define RSP_WAIT 0x00
#define RSP_LMSB 0x01
#define RSP_LLSB 0x02
#define RSP_RDDATA 0x03


class XBee900HP {
public:
  uint8_t rcvSize;
  uint8_t *rcvBuffer;
  XBee900HP(HardwareSerial *serial);
  bool init();
  void listen();
  void sendTo64RAW(uint32_t addr_high, uint32_t addr_low, uint8_t* data, uint16_t mSize);
  void sendTo64(uint32_t addr_high, uint32_t addr_low, char* data);
  void onFrameReceived(void (*handler)());
private:  
  uint8_t seq;
  uint8_t xBeeStatus;
  char *buffer;
  uint16_t buffPos;
  
  uint8_t d; // Temporary holder for the data
  bool responseFound; // Flag to stop reading
  uint16_t lMSB,lLSB; // Packet length (MSB,LSB)
  int packetSize; // Packet size built from the length
  int checkSum;
  uint32_t lastData;
  uint32_t lastSerialData;
  uint8_t readStatus;
  
  void (*frameReceivedHandler)();
  
  // Serial data processing functions
  bool waitFor(const char *response, void (*command)());
  bool waitFor(const char *response);
  
  void flush_buffer();
  void append_buffer(char c);
  
  void escapeAndWrite(uint8_t &data);
  
  HardwareSerial * _HardSerial;
  
  bool escapeNext;
};


