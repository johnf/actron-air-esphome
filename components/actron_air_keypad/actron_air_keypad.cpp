#include "actron_air_keypad.h"

namespace esphome {
namespace actron_air_keypad {

static const char *const TAG = "actron_air_keypad";

// Decode 7-segment pattern (GFEDCBA) to character
//    AAA
//   F   B
//    GGG
//   E   C
//    DDD
char ActronAirKeypad::decode_digit(uint8_t pattern) {
  switch (pattern) {
  case 0x3F:
    return '0'; // ABCDEF
  case 0x06:
    return '1'; // BC
  case 0x5B:
    return '2'; // ABDEG
  case 0x4F:
    return '3'; // ABCDG
  case 0x66:
    return '4'; // BCFG
  case 0x6D:
    return '5'; // ACDFG
  case 0x07:
    return '7'; // ABC
  case 0x7C:
    return '6'; // CDEFG (no A)
  case 0x7F:
    return '8'; // ABCDEFG
  case 0x67:
    return '9'; // ABCFG (no D)

  case 0x00:
    return ' '; // blank
  case 0x40:
    return '-'; // G (dash)

  case 0x79:
    return 'E'; // ADEFG
  case 0x73:
    return 'P'; // ABEFG

  default:
    ESP_LOGE(TAG, "Unknown 7-segment pattern: 0x%02X", pattern);
    return '?';
  }
}

float ActronAirKeypad::get_display_value() const {
  using L = LedIndex;

  // Extract segment bits for each digit (GFEDCBA pattern)
  auto extract = [this](L a, L b, L c, L d, L e, L f, L g) {
    return static_cast<uint8_t>((get_pulse(g) << 6) | (get_pulse(f) << 5) |
                                (get_pulse(e) << 4) | (get_pulse(d) << 3) |
                                (get_pulse(c) << 2) | (get_pulse(b) << 1) |
                                get_pulse(a));
  };

  uint8_t d1 = extract(L::DIGIT1_A, L::DIGIT1_B, L::DIGIT1_C, L::DIGIT1_D,
                       L::DIGIT1_E, L::DIGIT1_F, L::DIGIT1_G);
  uint8_t d2 = extract(L::DIGIT2_A, L::DIGIT2_B, L::DIGIT2_C, L::DIGIT2_D,
                       L::DIGIT2_E, L::DIGIT2_F, L::DIGIT2_G);
  uint8_t d3 = extract(L::DIGIT3_A, L::DIGIT3_B, L::DIGIT3_C, L::DIGIT3_D,
                       L::DIGIT3_E, L::DIGIT3_F, L::DIGIT3_G);

  char c1 = decode_digit(d1);
  char c2 = decode_digit(d2);
  char c3 = decode_digit(d3);

  if (!std::isdigit(static_cast<unsigned char>(c1)) ||
      !std::isdigit(static_cast<unsigned char>(c2)) ||
      !std::isdigit(static_cast<unsigned char>(c3))) {
    return -1.0f;
  }

  int value = (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0');
  float display_value = static_cast<float>(value);

  if (get_pulse(L::DP)) {
    display_value *= 0.1f;
  }

  return display_value;
}

void IRAM_ATTR ActronAirKeypad::handle_interrupt(ActronAirKeypad *arg) {
  auto now_us = micros();
  unsigned long delta_us = now_us - arg->last_intr_us_;
  arg->last_intr_us_ = now_us;

  if (delta_us > FRAME_BOUNDARY_US) {
    arg->has_data_error_ = false;

    return;
  }

  if (delta_us >= START_CONDITION_US) {
    arg->num_low_pulses_ = 0;

    return;
  }

  if (arg->num_low_pulses_ >= NPULSE) {
    arg->has_data_error_ = true;
    uint32_t count = arg->error_count_;
    if (count < UINT32_MAX) {
      arg->error_count_ = count + 1;
    }
  }

  arg->pulse_vec_[arg->num_low_pulses_] = delta_us < PULSE_THRESHOLD_US;
  arg->num_low_pulses_ = static_cast<uint8_t>(arg->num_low_pulses_ + 1);
  arg->do_work_ = true;
}

void ActronAirKeypad::setup() {
  if (!pin_) {
    ESP_LOGE(TAG, "Pin not configured");

    return;
  }

  ESP_LOGD(TAG, "Setting up on pin");
  pin_->setup();

  auto *internal_pin = static_cast<InternalGPIOPin *>(pin_);
  ESP_LOGD(TAG, "Setting up interrupt");
  internal_pin->attach_interrupt(handle_interrupt, this,
                                 gpio::INTERRUPT_FALLING_EDGE);
}

void ActronAirKeypad::loop() {
  unsigned long now_us = micros();

  if (do_work_) {
    do_work_ = false;
    last_work_us_ = now_us;

    return;
  }

  unsigned long delta_us = now_us - last_work_us_;
  if (delta_us > FRAME_TIMEOUT_US && num_low_pulses_) {
    if (num_low_pulses_ == NPULSE && !has_data_error_) {
      InterruptLock lock;
      const char *vec = const_cast<const char *>(pulse_vec_);
      if (std::memcmp(pulses_.data(), vec, NPULSE) != 0) {
        has_new_data_ = true;
        std::memcpy(pulses_.data(), vec, NPULSE);
      }
    } else {
      ESP_LOGVV(TAG, "Only %u bits received (or data error: %u)",
                num_low_pulses_, has_data_error_ ? 1 : 0);
    }

    num_low_pulses_ = 0;
    last_work_us_ = now_us;
  }

  if (!has_new_data_) {
    return;
  }

  has_new_data_ = false;
  ESP_LOGD(TAG, "New data available");

  if (bit_string_) {
    std::string text;
    text.reserve(NPULSE);
    for (std::size_t i = 0; i < NPULSE; ++i) {
      text += (pulses_[i] ? '1' : '0');
    }
    bit_string_->publish_state(text);
  }

  if (setpoint_temp_) {
    setpoint_temp_->publish_state(get_display_value());
  }

  if (error_count_sensor_) {
    error_count_sensor_->publish_state(error_count_);
  }

  if (room_)
    room_->publish_state(get_pulse(LedIndex::ROOM));
  if (fan_cont_)
    fan_cont_->publish_state(get_pulse(LedIndex::FAN_CONT));
  if (fan_high_)
    fan_high_->publish_state(get_pulse(LedIndex::FAN_HIGH));
  if (fan_mid_)
    fan_mid_->publish_state(get_pulse(LedIndex::FAN_MID));
  if (fan_low_)
    fan_low_->publish_state(get_pulse(LedIndex::FAN_LOW));
  if (cool_)
    cool_->publish_state(get_pulse(LedIndex::COOL));
  if (auto_mode_)
    auto_mode_->publish_state(get_pulse(LedIndex::AUTO_MODE));
  if (heat_)
    heat_->publish_state(get_pulse(LedIndex::HEAT));
  if (run_)
    run_->publish_state(get_pulse(LedIndex::RUN));
  if (timer_)
    timer_->publish_state(get_pulse(LedIndex::TIMER));
  if (inside_)
    inside_->publish_state(get_pulse(LedIndex::INSIDE));
  if (zone1_)
    zone1_->publish_state(get_pulse(LedIndex::ZONE1));
  if (zone2_)
    zone2_->publish_state(get_pulse(LedIndex::ZONE2));
  if (zone3_)
    zone3_->publish_state(get_pulse(LedIndex::ZONE3));
  if (zone4_)
    zone4_->publish_state(get_pulse(LedIndex::ZONE4));
  if (zone5_)
    zone5_->publish_state(get_pulse(LedIndex::ZONE5));
  if (zone6_)
    zone6_->publish_state(get_pulse(LedIndex::ZONE6));
  if (zone7_)
    zone7_->publish_state(get_pulse(LedIndex::ZONE7));
}

void ActronAirKeypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron Air Keypad:");
  LOG_PIN("  Pin: ", this->pin_);
}

} // namespace actron_air_keypad
} // namespace esphome
