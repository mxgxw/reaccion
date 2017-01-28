typedef struct rx_frame_protocol_0 {
  uint16_t version = 0x00;
  uint8_t category;
  uint8_t level;
  uint32_t lon;
  uint32_t lat;
  uint8_t month;
  uint8_t date;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

typedef struct rx_frame_protocol_1{
  uint16_t version = 0x01;
  uint32_t origin;
  uint8_t category;
  uint8_t level;
  uint32_t lon;
  uint32_t lat;
  uint8_t month;
  uint8_t date;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  int32_t temp;
  int32_t pressure;
  uint32_t running;
};


