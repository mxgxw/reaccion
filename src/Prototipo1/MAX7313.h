class MAX7313 {
private:
  uint8_t address;
  uint8_t int_pin;
public:
  static void handle_interrupt();
  MAX7313(uint8_t addr, uint8_t int_pin);
  bool init();
  uint8_t digitalRead(uint8_t pin);
};

