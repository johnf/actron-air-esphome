#include <cctype>
#include <cstring>

#include <esphome/core/hal.h>
#include <esphome/core/helpers.h>
#include <esphome/core/log.h>

#include "led_protocol.h"

namespace esphome {
namespace actron_air_keypad {

static const char *const TAG = "actron_air_keypad";

void IRAM_ATTR LedProtocol::handle_interrupt() {
  auto now_us = micros();
  unsigned long delta_us = now_us - last_intr_us_;
  last_intr_us_ = now_us;

  // Frame boundary reset
  if (delta_us > timing::FRAME_BOUNDARY_US) {
    has_data_error_ = false;
    return;
  }

  // Start condition
  if (delta_us >= timing::START_CONDITION_US) {
    num_low_pulses_ = 0;
    return;
  }

  if (num_low_pulses_ >= NPULSE) {
    has_data_error_ = true;
    // Explicit read-modify-write to avoid deprecated volatile increment (C++20)
    uint32_t count = error_count_;
    if (count < UINT32_MAX) {
      error_count_ = count + 1;
    }

    return;
  }

  pulse_vec_[num_low_pulses_] = delta_us < timing::PULSE_THRESHOLD_US;
  // Explicit read-modify-write to avoid deprecated volatile increment (C++20)
  num_low_pulses_ = static_cast<uint8_t>(num_low_pulses_ + 1);

  do_work_ = true;
}

void LedProtocol::main_loop() {
  unsigned long now_us = micros();
  if (do_work_) {
    do_work_ = false;
    last_work_us_ = now_us;

    return;
  }

  unsigned long delta_us = now_us - last_work_us_;
  if (delta_us > timing::FRAME_TIMEOUT_US && num_low_pulses_) {
    if (num_low_pulses_ == NPULSE && !has_data_error_) {
      // Use InterruptLock to prevent race condition during copy.
      // The ISR could modify pulse_vec_ while we're reading it.
      InterruptLock lock;

      // Cast away volatile for memcmp/memcpy (safe under InterruptLock)
      const char *vec = const_cast<const char *>(pulse_vec_);
      if (std::memcmp(pulses_.data(), vec, NPULSE) != 0) {
        has_new_data_ = true;
        std::memcpy(pulses_.data(), vec, NPULSE);
      }
    } else {
      ESP_LOGD(TAG, "Only %u bits received (or data error)", num_low_pulses_);
    }

    num_low_pulses_ = 0;
    last_work_us_ = now_us;
  }
}

// Seven-segment display patterns: bits are GFEDCBA (bit 6 = G, bit 0 = A)
// Standard 7-segment layout:
//    AAA
//   F   B
//    GGG
//   E   C
//    DDD
static constexpr struct {
  uint8_t pattern;
  char digit;
} SEGMENT_MAP[] = {
    {0x3F, '0'},  // ABCDEF
    {0x06, '1'},  // BC
    {0x5B, '2'},  // ABDEG
    {0x4F, '3'},  // ABCDG
    {0x66, '4'},  // BCFG
    {0x6D, '5'},  // ACDFG
    {0x7C, '6'},  // CDEFG (some displays omit A)
    {0x07, '7'},  // ABC
    {0x7F, '8'},  // ABCDEFG
    {0x67, '9'},  // ABCFG (some displays include D)
    {0x73, 'P'},  // ABEFG
    {0x79, 'E'},  // ADEFG
};

char LedProtocol::decode_digit(uint8_t segment_bits) {
  for (const auto &entry : SEGMENT_MAP) {
    if (entry.pattern == segment_bits) {
      return entry.digit;
    }
  }

  return '?';
}

uint8_t LedProtocol::extract_digit_bits(LedIndex a, LedIndex b, LedIndex c,
                                        LedIndex d, LedIndex e, LedIndex f,
                                        LedIndex g) const {
  // Extract each segment bit and combine into GFEDCBA pattern
  return static_cast<uint8_t>(
      (get_pulse(g) << 6) | (get_pulse(f) << 5) | (get_pulse(e) << 4) |
      (get_pulse(d) << 3) | (get_pulse(c) << 2) | (get_pulse(b) << 1) |
      get_pulse(a));
}

float LedProtocol::get_display_value() const {
  using L = LedIndex;

  uint8_t digit1_bits = extract_digit_bits(L::DIGIT1_A, L::DIGIT1_B, L::DIGIT1_C,
                                           L::DIGIT1_D, L::DIGIT1_E, L::DIGIT1_F,
                                           L::DIGIT1_G);
  uint8_t digit2_bits = extract_digit_bits(L::DIGIT2_A, L::DIGIT2_B, L::DIGIT2_C,
                                           L::DIGIT2_D, L::DIGIT2_E, L::DIGIT2_F,
                                           L::DIGIT2_G);
  uint8_t digit3_bits = extract_digit_bits(L::DIGIT3_A, L::DIGIT3_B, L::DIGIT3_C,
                                           L::DIGIT3_D, L::DIGIT3_E, L::DIGIT3_F,
                                           L::DIGIT3_G);

  char c1 = decode_digit(digit1_bits);
  char c2 = decode_digit(digit2_bits);
  char c3 = decode_digit(digit3_bits);

  // Check all characters are digits
  if (!std::isdigit(static_cast<unsigned char>(c1)) ||
      !std::isdigit(static_cast<unsigned char>(c2)) ||
      !std::isdigit(static_cast<unsigned char>(c3))) {
    return -1.0f;
  }

  // Convert to integer using arithmetic (avoids std::stof exception risk)
  int value = (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0');
  float display_value = static_cast<float>(value);

  // Apply decimal point if present
  if (get_pulse(L::DP)) {
    display_value *= 0.1f;
  }

  return display_value;
}

} // namespace actron_air_keypad
} // namespace esphome
