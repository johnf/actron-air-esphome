#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"

#include "frame_decoder.h"

namespace esphome {
namespace actron_air_esphome {

// Displayed values outside this range (timer, lamp test) aren't a setpoint.
constexpr float SETPOINT_MIN_C = 5.0f;
constexpr float SETPOINT_MAX_C = 40.0f;

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
  ZONE_1 = 21,
  ZONE_2 = 14,
  ZONE_3 = 12,
  ZONE_4 = 13,
  ZONE_5 = 2,
  ZONE_6 = 6,
  ZONE_7 = 5,
  ZONE_8 = 4,

  // Other status indicators
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

// Binary sensor identifiers for array indexing
enum class BinarySensorId : uint8_t {
  FAN_CONT,
  FAN_HIGH,
  FAN_MID,
  FAN_LOW,
  COOL,
  AUTO_MODE,
  HEAT,
  RUN,
  TIMER,
  INSIDE,
  ZONE_1,
  ZONE_2,
  ZONE_3,
  ZONE_4,
  ZONE_5,
  ZONE_6,
  ZONE_7,
  ZONE_8,
  COUNT  // Must be last - used for array sizing
};

// Number of binary sensors
constexpr std::size_t BINARY_SENSOR_COUNT =
    static_cast<std::size_t>(BinarySensorId::COUNT);

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

  void set_binary_sensor(BinarySensorId id, binary_sensor::BinarySensor *s) {
    binary_sensors_[static_cast<std::size_t>(id)] = s;
  }

private:
  static void IRAM_ATTR handle_interrupt(ActronAirKeypad *arg);
  float get_display_value() const;
  static char decode_digit(uint8_t segment_bits);

  bool get_pulse(LedIndex idx) const {
    return (pulses_ >> static_cast<std::size_t>(idx)) & 1;
  }

  GPIOPin *pin_{nullptr};

  // Sensors
  sensor::Sensor *setpoint_temp_{nullptr};
  sensor::Sensor *error_count_sensor_{nullptr};
  text_sensor::TextSensor *bit_string_{nullptr};

  std::array<binary_sensor::BinarySensor *, BINARY_SENSOR_COUNT>
      binary_sensors_{};

  // Main loop only
  FrameConfirmer confirmer_;
  uint64_t pulses_{0};  // Last published frame
  uint32_t published_error_count_{~0U};

  // ISR only
  FrameDecoder decoder_;

  // ISR writes, loop reads
  std::atomic<uint64_t> frame_bits_{0};
  std::atomic<bool> frame_ready_{false};
  std::atomic<uint32_t> error_count_{0};
};

} // namespace actron_air_esphome
} // namespace esphome
