typedef struct packet_header {
  uint8_t origin;
  uint8_t dest;
  uint8_t last_hop;
  uint8_t next_hop;
  uint8_t jumps;
};

typedef struct data_frame_v0 {
  uint8_t frame_version = 0;
  uint8_t alert_lvl;
};

