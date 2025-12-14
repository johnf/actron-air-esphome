#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include "esphome/core/gpio.h"
#include "led_protocol.h"

namespace esphome {
namespace actron_air_keypad {

/// ESPHome component that decodes Actron Air keypad display data.
///
/// Captures a pulse train from the keypad's display wire and decodes it to
/// extract temperature setpoint, mode indicators, fan speeds, and zone states.
/// Requires an InternalGPIOPin for interrupt-based pulse capture.
class ActronAirKeypad : public Component {
public:
  ActronAirKeypad() = default;
  ~ActronAirKeypad();

  // Delete copy operations
  ActronAirKeypad(const ActronAirKeypad &) = delete;
  ActronAirKeypad &operator=(const ActronAirKeypad &) = delete;

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  static void handle_interrupt(ActronAirKeypad *arg);

  void set_pin(GPIOPin *pin) { this->pin_ = pin; }

  void set_setpoint_temp_sensor(sensor::Sensor *sensor) {
    this->setpoint_temp_ = sensor;
  }

  void set_room_sensor(binary_sensor::BinarySensor *sensor) {
    this->room_ = sensor;
  }
  void set_fan_cont_sensor(binary_sensor::BinarySensor *sensor) {
    this->fan_cont_ = sensor;
  }
  void set_fan_high_sensor(binary_sensor::BinarySensor *sensor) {
    this->fan_high_ = sensor;
  }
  void set_fan_mid_sensor(binary_sensor::BinarySensor *sensor) {
    this->fan_mid_ = sensor;
  }
  void set_fan_low_sensor(binary_sensor::BinarySensor *sensor) {
    this->fan_low_ = sensor;
  }
  void set_cool_sensor(binary_sensor::BinarySensor *sensor) {
    this->cool_ = sensor;
  }
  void set_auto_mode_sensor(binary_sensor::BinarySensor *sensor) {
    this->auto_mode_ = sensor;
  }
  void set_heat_sensor(binary_sensor::BinarySensor *sensor) {
    this->heat_ = sensor;
  }
  void set_run_sensor(binary_sensor::BinarySensor *sensor) {
    this->run_ = sensor;
  }
  void set_timer_sensor(binary_sensor::BinarySensor *sensor) {
    this->timer_ = sensor;
  }
  void set_inside_sensor(binary_sensor::BinarySensor *sensor) {
    this->inside_ = sensor;
  }
  void set_zone1_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone1_ = sensor;
  }
  void set_zone2_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone2_ = sensor;
  }
  void set_zone3_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone3_ = sensor;
  }
  void set_zone4_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone4_ = sensor;
  }
  void set_zone5_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone5_ = sensor;
  }
  void set_zone6_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone6_ = sensor;
  }
  void set_zone7_sensor(binary_sensor::BinarySensor *sensor) {
    this->zone7_ = sensor;
  }

  void set_bit_string_sensor(text_sensor::TextSensor *sensor) {
    this->bit_string_ = sensor;
  }
  void set_error_count_sensor(sensor::Sensor *sensor) {
    this->error_count_ = sensor;
  }

protected:
  GPIOPin *pin_{nullptr};
  LedProtocol led_protocol_;

  sensor::Sensor *setpoint_temp_{nullptr};

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

  text_sensor::TextSensor *bit_string_{nullptr};
  sensor::Sensor *error_count_{nullptr};
};

} // namespace actron_air_keypad
} // namespace esphome
