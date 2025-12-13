#pragma once

#include <cstdint>
#include <esphome/core/hal.h>

namespace esphome {
namespace actron_air_keypad {

#define NPULSE 40

class LedProtocol {
public:
  enum ClassStatLeds {
    COOL = 0,
    AUTO = 1,
    ZONE5 = 2,
    RUN = 3,
    ROOM = 4,
    ZONE7 = 5,
    ZONE6 = 6,
    TIMER = 7,
    FAN_CONT = 8,
    FAN_HIGH = 9,
    FAN_MID = 10,
    FAN_LOW = 11,
    ZONE3 = 12,
    ZONE4 = 13,
    ZONE2 = 14,
    HEAT = 15,
    _3C = 16,
    _3F = 17,
    _3G = 18,
    _3B = 19,
    _3A = 20,
    ZONE1 = 21,
    _3E = 22,
    _3D = 23,
    _2B = 24,
    _2F = 25,
    _2G = 26,
    _2E = 27,
    DP = 28,
    _2C = 29,
    _2D = 30,
    _2A = 31,
    _1D = 32,
    INSIDE = 33,
    _1C = 34,
    _1B = 35,
    _1E = 36,
    _1G = 37,
    _1F = 38,
    _1A = 39,
  };

  volatile unsigned char dbg_nerr{0};
  bool newdata{false};
  char p[NPULSE]{};

  float get_display_value();
  void mloop();
  void IRAM_ATTR handle_interrupt();

private:
  unsigned long last_intr_us_{0};
  unsigned long last_work_{0};
  char pulse_vec_[NPULSE]{};
  volatile unsigned char nlow_{0};
  volatile unsigned char nbits_{0};
  volatile bool do_work_{false};
  bool data_error_{false};

  void handleIntr();
  char decode_digit(uint8_t hex_value);
};

extern LedProtocol ledProto;

} // namespace actron_air_keypad
} // namespace esphome
