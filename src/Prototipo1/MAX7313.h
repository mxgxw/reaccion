class MAX7313 {
private:
  int address;
  int int_pin;
public:
  static void handle_interrupt();
  MAX7313(int addr, int int_pin);
  bool init();
  uint8_t digitalRead(int pin);
  void digitalWrite(uint8_t pin, uint8_t val);
};


