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

typedef struct rx_shared_key_change_req {
  uint16_t version = 0xFFFF;
  uint16_t version = 0x0000;
  uint8_t shared_key[20];
};

typedef struct rx_gw_broadcast_announce {
  uint16_t version = 0xFFFF;
  uint16_t version = 0x0001;
  uint32_t addr_high = 0x0001;
  uint32_t addr_low = 0x0001;
};


