#include <cstring>
#include <esphome/core/hal.h>
#include <esphome/core/log.h>

#include "led_protocol.h"

namespace esphome {
namespace actron_air_keypad {

static const char *const TAG = "actron_air_keypad";

LedProtocol ledProto;

void IRAM_ATTR LedProtocol::handle_interrupt() {
  ESP_LOGD(TAG, "Interrupt received");
  auto nowu = micros();
  unsigned long dtu = nowu - last_intr_us_;
  last_intr_us_ = nowu;

  if (dtu > 3500) {
    data_error_ = false;
    return;
  }

  if (dtu >= 2700) {
    nlow_ = 0;
  } else {
    if (nlow_ >= NPULSE) {
      data_error_ = true;
      dbg_nerr = dbg_nerr + 1;
      nlow_ = NPULSE;
    }
    pulse_vec_[nlow_] = dtu < 1000;
    nlow_ = nlow_ + 1;
    do_work_ = 1;
  }
  ESP_LOGD(TAG, "Interrupt processing done");
}

char LedProtocol::decode_digit(uint8_t hex_value) {
  switch (hex_value) {
  case 0x3F:
    return '0';
  case 0x06:
    return '1';
  case 0x5B:
    return '2';
  case 0x4F:
    return '3';
  case 0x66:
    return '4';
  case 0x6D:
    return '5';
  case 0x7C:
    return '6';
  case 0x07:
    return '7';
  case 0x7F:
    return '8';
  case 0x67:
    return '9';
  case 0x73:
    return 'P';
  default:
    return '?';
  }
}

float LedProtocol::get_display_value() {
  uint8_t digit1_bits = (p[_1G] << 6) | (p[_1F] << 5) | (p[_1E] << 4) |
                        (p[_1D] << 3) | (p[_1C] << 2) | (p[_1B] << 1) | p[_1A];
  uint8_t digit2_bits = (p[_2G] << 6) | (p[_2F] << 5) | (p[_2E] << 4) |
                        (p[_2D] << 3) | (p[_2C] << 2) | (p[_2B] << 1) | p[_2A];
  uint8_t digit3_bits = (p[_3G] << 6) | (p[_3F] << 5) | (p[_3E] << 4) |
                        (p[_3D] << 3) | (p[_3C] << 2) | (p[_3B] << 1) | p[_3A];

  std::string display_str;
  display_str += decode_digit(digit1_bits);
  display_str += decode_digit(digit2_bits);
  display_str += decode_digit(digit3_bits);

  for (char c : display_str) {
    if (!isdigit(c))
      return -1.0f;
  }

  float display_value = std::stof(display_str);
  if (p[DP])
    display_value *= 0.1f;
  return display_value;
}

void LedProtocol::mloop() {
  unsigned long now = micros();
  if (do_work_) {
    ESP_LOGD(TAG, "Processing data");
    do_work_ = 0;
    last_work_ = now;

    return;
  }

  unsigned long dt = now - last_work_;
  if (dt > 40000 && nlow_) {
    ESP_LOGD(TAG, "Finalizing data");
    nbits_ = nlow_;
    nlow_ = 0;
    if (nbits_ == 40 && !data_error_) {
      ESP_LOGD(TAG, "40 bits received");
      if (memcmp(p, pulse_vec_, sizeof p) != 0) {
        ESP_LOGD(TAG, "Data changed");
        newdata = true;
        memcpy(p, pulse_vec_, sizeof p);
      }
    } else {
      ESP_LOGD(TAG, "Only %d bits received (Or data error)", nbits_);
    }
    last_work_ = now;
  }
}

} // namespace actron_air_keypad
} // namespace esphome
