#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <esphome/core/hal.h>

namespace esphome {
namespace actron_air_keypad {

// Protocol timing constants (microseconds)
// The Actron Air keypad sends a pulse train where timing encodes the data.
namespace timing {
// Time gap indicating start of new frame (no pulse for >3.5ms resets state)
constexpr unsigned long FRAME_BOUNDARY_US = 3500;
// Preamble pulse duration indicating start condition (~2.7ms pulse)
constexpr unsigned long START_CONDITION_US = 2700;
// Threshold to distinguish '0' (long pulse >=1ms) from '1' (short pulse <1ms)
constexpr unsigned long PULSE_THRESHOLD_US = 1000;
// Maximum time to wait after last pulse before processing complete frame
constexpr unsigned long FRAME_TIMEOUT_US = 40000;
} // namespace timing

// Protocol frame size (40 bits per frame)
constexpr std::size_t NPULSE = 40;

// LED/segment bit indices in the protocol frame.
// Status LEDs (modes, fan speeds, zones) and 7-segment display segments.
// Digit segments follow standard naming: A-G where A is top, G is middle.
enum class LedIndex : std::size_t {
  // Mode indicators
  COOL = 0,
  AUTO_MODE = 1,
  RUN = 3,
  HEAT = 15,

  // Fan speed indicators
  FAN_CONT = 8,
  FAN_HIGH = 9,
  FAN_MID = 10,
  FAN_LOW = 11,

  // Zone indicators (1-7)
  ZONE1 = 21,
  ZONE2 = 14,
  ZONE3 = 12,
  ZONE4 = 13,
  ZONE5 = 2,
  ZONE6 = 6,
  ZONE7 = 5,

  // Other status indicators
  ROOM = 4,
  TIMER = 7,
  INSIDE = 33,

  // 7-segment display - Digit 1 (leftmost)
  DIGIT1_A = 39,
  DIGIT1_B = 35,
  DIGIT1_C = 34,
  DIGIT1_D = 32,
  DIGIT1_E = 36,
  DIGIT1_F = 38,
  DIGIT1_G = 37,

  // 7-segment display - Digit 2 (middle)
  DIGIT2_A = 31,
  DIGIT2_B = 24,
  DIGIT2_C = 29,
  DIGIT2_D = 30,
  DIGIT2_E = 27,
  DIGIT2_F = 25,
  DIGIT2_G = 26,

  // 7-segment display - Digit 3 (rightmost)
  DIGIT3_A = 20,
  DIGIT3_B = 19,
  DIGIT3_C = 16,
  DIGIT3_D = 23,
  DIGIT3_E = 22,
  DIGIT3_F = 17,
  DIGIT3_G = 18,

  // Decimal point (between digit 2 and 3)
  DP = 28,
};

/// Decodes the pulse train protocol from an Actron Air keypad display.
/// The protocol consists of 40-bit frames sent as timed pulses on a single
/// wire.
class LedProtocol {
public:
  // Data access methods
  [[nodiscard]] uint32_t get_error_count() const { return error_count_; }
  [[nodiscard]] bool has_new_data() const { return has_new_data_; }
  void clear_new_data() { has_new_data_ = false; }

  /// Get the state of a specific LED/segment bit.
  /// Returns 0 if index is out of bounds.
  [[nodiscard]] char get_pulse(std::size_t index) const {
    return index < NPULSE ? pulses_[index] : 0;
  }

  /// Get the state of a specific LED by enum index.
  [[nodiscard]] char get_pulse(LedIndex index) const {
    return get_pulse(static_cast<std::size_t>(index));
  }

  [[nodiscard]] const std::array<char, NPULSE> &get_pulses() const {
    return pulses_;
  }

  /// Decode the 3-digit 7-segment display value.
  /// Returns -1.0f if the display shows non-numeric characters.
  [[nodiscard]] float get_display_value() const;

  /// Process buffered interrupt data. Call from main loop.
  void main_loop();

  /// ISR handler - record pulse timing. Must be called from interrupt context.
  void handle_interrupt();

private:
  // State accessible via getters
  volatile uint32_t error_count_{0};
  bool has_new_data_{false};
  std::array<char, NPULSE> pulses_{};

  // Internal ISR state (volatile as accessed from interrupt context)
  // Note: Using C-style array for pulse_vec_ because std::array::operator[]
  // is not volatile-qualified, making it incompatible with volatile arrays.
  volatile unsigned long last_intr_us_{0};
  volatile unsigned long last_work_us_{0};
  volatile char pulse_vec_[NPULSE]{};
  volatile uint8_t num_low_pulses_{0};
  volatile bool do_work_{false};
  volatile bool has_data_error_{false};

  /// Decode a 7-segment pattern to a character.
  static char decode_digit(uint8_t segment_bits);

  /// Extract the 7 segment bits for a digit from the pulse array.
  [[nodiscard]] uint8_t extract_digit_bits(LedIndex a, LedIndex b, LedIndex c,
                                           LedIndex d, LedIndex e, LedIndex f,
                                           LedIndex g) const;
};

} // namespace actron_air_keypad
} // namespace esphome
