#pragma once

// #include <array>
#include <cstddef>
// #include <cstdint>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace actron_air_keypad {

// Protocol timing constants (microseconds)
constexpr unsigned long FRAME_BOUNDARY_US = 3500;
constexpr unsigned long START_CONDITION_US = 2700;
constexpr unsigned long PULSE_THRESHOLD_US = 1000;
constexpr unsigned long FRAME_TIMEOUT_US = 40000;

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

/// ESPHome component that decodes Actron Air keypad display data.
///
/// Captures a pulse train from the keypad's display wire and decodes it to
/// extract temperature setpoint, mode indicators, fan speeds, and zone states.
class ActronAirKeypad : public Component {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void set_pin(GPIOPin *pin) { pin_ = pin; }

  // Sensor setters
  void set_setpoint_temp_sensor(sensor::Sensor *s) { setpoint_temp_ = s; }
  void set_error_count_sensor(sensor::Sensor *s) { error_count_sensor_ = s; }
  void set_bit_string_sensor(text_sensor::TextSensor *s) { bit_string_ = s; }

  void set_room_sensor(binary_sensor::BinarySensor *s) { room_ = s; }
  void set_fan_cont_sensor(binary_sensor::BinarySensor *s) { fan_cont_ = s; }
  void set_fan_high_sensor(binary_sensor::BinarySensor *s) { fan_high_ = s; }
  void set_fan_mid_sensor(binary_sensor::BinarySensor *s) { fan_mid_ = s; }
  void set_fan_low_sensor(binary_sensor::BinarySensor *s) { fan_low_ = s; }
  void set_cool_sensor(binary_sensor::BinarySensor *s) { cool_ = s; }
  void set_auto_mode_sensor(binary_sensor::BinarySensor *s) { auto_mode_ = s; }
  void set_heat_sensor(binary_sensor::BinarySensor *s) { heat_ = s; }
  void set_run_sensor(binary_sensor::BinarySensor *s) { run_ = s; }
  void set_timer_sensor(binary_sensor::BinarySensor *s) { timer_ = s; }
  void set_inside_sensor(binary_sensor::BinarySensor *s) { inside_ = s; }
  void set_zone1_sensor(binary_sensor::BinarySensor *s) { zone1_ = s; }
  void set_zone2_sensor(binary_sensor::BinarySensor *s) { zone2_ = s; }
  void set_zone3_sensor(binary_sensor::BinarySensor *s) { zone3_ = s; }
  void set_zone4_sensor(binary_sensor::BinarySensor *s) { zone4_ = s; }
  void set_zone5_sensor(binary_sensor::BinarySensor *s) { zone5_ = s; }
  void set_zone6_sensor(binary_sensor::BinarySensor *s) { zone6_ = s; }
  void set_zone7_sensor(binary_sensor::BinarySensor *s) { zone7_ = s; }

private:
  static void IRAM_ATTR handle_interrupt(ActronAirKeypad *arg);
  void process_frame();
  float get_display_value() const;
  static char decode_digit(uint8_t segment_bits);

  char get_pulse(LedIndex idx) const {
    return pulses_[static_cast<std::size_t>(idx)];
  }

  GPIOPin *pin_{nullptr};

  // Sensors
  sensor::Sensor *setpoint_temp_{nullptr};
  sensor::Sensor *error_count_sensor_{nullptr};
  text_sensor::TextSensor *bit_string_{nullptr};

  binary_sensor::BinarySensor *room_{nullptr};
  binary_sensor::BinarySensor *fan_cont_{nullptr};
  binary_sensor::BinarySensor *fan_high_{nullptr};
  binary_sensor::BinarySensor *fan_mid_{nullptr};
  binary_sensor::BinarySensor *fan_low_{nullptr};
  binary_sensor::BinarySensor *cool_{nullptr};
  binary_sensor::BinarySensor *auto_mode_{nullptr};
  binary_sensor::BinarySensor *heat_{nullptr};
  binary_sensor::BinarySensor *run_{nullptr};
  binary_sensor::BinarySensor *timer_{nullptr};
  binary_sensor::BinarySensor *inside_{nullptr};
  binary_sensor::BinarySensor *zone1_{nullptr};
  binary_sensor::BinarySensor *zone2_{nullptr};
  binary_sensor::BinarySensor *zone3_{nullptr};
  binary_sensor::BinarySensor *zone4_{nullptr};
  binary_sensor::BinarySensor *zone5_{nullptr};
  binary_sensor::BinarySensor *zone6_{nullptr};
  binary_sensor::BinarySensor *zone7_{nullptr};

  // Protocol state
  std::array<char, NPULSE> pulses_{};
  bool has_new_data_{false};

  // ISR state (volatile)
  volatile unsigned long last_intr_us_{0};
  volatile unsigned long last_work_us_{0};
  volatile char pulse_vec_[NPULSE]{};
  volatile uint8_t num_low_pulses_{0};
  volatile uint32_t error_count_{0};
  volatile bool do_work_{false};
  volatile bool has_data_error_{false};
};

} // namespace actron_air_keypad
} // namespace esphome
