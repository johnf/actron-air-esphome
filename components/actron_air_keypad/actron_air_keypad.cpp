#include "actron_air_keypad.h"

#include <esphome/core/gpio.h>
#include <esphome/core/log.h>

namespace esphome {
namespace actron_air_keypad {

static const char *const TAG = "actron_air_keypad";

namespace {

void publish_binary(binary_sensor::BinarySensor *sensor, bool state) {
  if (sensor) {
    sensor->publish_state(state);
  }
}

}  // namespace

void IRAM_ATTR ActronAirKeypad::handle_interrupt(ActronAirKeypad *arg) {
  if (arg) {
    arg->led_protocol_.handle_interrupt();
  }
}

ActronAirKeypad::~ActronAirKeypad() {
  if (pin_) {
    // Note: This component requires InternalGPIOPin for interrupt support.
    // The ESPHome schema should ensure only internal GPIO pins are used.
    auto *internal_pin = static_cast<InternalGPIOPin *>(pin_);
    internal_pin->detach_interrupt();
  }
}

void ActronAirKeypad::setup() {
  if (!pin_) {
    ESP_LOGE(TAG, "Pin not configured");

    return;
  }

  ESP_LOGD(TAG, "Setting up on pin");
  pin_->setup();

  // Note: This component requires InternalGPIOPin for interrupt support.
  // The ESPHome schema should ensure only internal GPIO pins are used.
  auto *internal_pin = static_cast<InternalGPIOPin *>(pin_);

  ESP_LOGD(TAG, "Setting up interrupt");
  internal_pin->attach_interrupt(handle_interrupt, this,
                                 gpio::INTERRUPT_FALLING_EDGE);
}

void ActronAirKeypad::loop() {
  led_protocol_.main_loop();

  if (!led_protocol_.has_new_data()) {
    return;
  }

  ESP_LOGD(TAG, "New data available");

  // Publish bit string for debugging
  if (bit_string_) {
    std::string text;
    text.reserve(NPULSE);
    for (std::size_t i = 0; i < NPULSE; ++i) {
      text += (led_protocol_.get_pulse(i) ? '1' : '0');
    }
    bit_string_->publish_state(text);
  }

  // Publish temperature
  if (setpoint_temp_) {
    setpoint_temp_->publish_state(led_protocol_.get_display_value());
  }

  // Publish error count
  if (error_count_) {
    error_count_->publish_state(led_protocol_.get_error_count());
  }

  // Publish binary sensors using enum class indices
  using L = LedIndex;
  publish_binary(room_, led_protocol_.get_pulse(L::ROOM));
  publish_binary(fan_cont_, led_protocol_.get_pulse(L::FAN_CONT));
  publish_binary(fan_high_, led_protocol_.get_pulse(L::FAN_HIGH));
  publish_binary(fan_mid_, led_protocol_.get_pulse(L::FAN_MID));
  publish_binary(fan_low_, led_protocol_.get_pulse(L::FAN_LOW));
  publish_binary(cool_, led_protocol_.get_pulse(L::COOL));
  publish_binary(auto_mode_, led_protocol_.get_pulse(L::AUTO_MODE));
  publish_binary(heat_, led_protocol_.get_pulse(L::HEAT));
  publish_binary(run_, led_protocol_.get_pulse(L::RUN));
  publish_binary(timer_, led_protocol_.get_pulse(L::TIMER));
  publish_binary(inside_, led_protocol_.get_pulse(L::INSIDE));
  publish_binary(zone1_, led_protocol_.get_pulse(L::ZONE1));
  publish_binary(zone2_, led_protocol_.get_pulse(L::ZONE2));
  publish_binary(zone3_, led_protocol_.get_pulse(L::ZONE3));
  publish_binary(zone4_, led_protocol_.get_pulse(L::ZONE4));
  publish_binary(zone5_, led_protocol_.get_pulse(L::ZONE5));
  publish_binary(zone6_, led_protocol_.get_pulse(L::ZONE6));
  publish_binary(zone7_, led_protocol_.get_pulse(L::ZONE7));

  led_protocol_.clear_new_data();
}

void ActronAirKeypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron Air Keypad:");
  LOG_PIN("  Pin: ", this->pin_);
}

} // namespace actron_air_keypad
} // namespace esphome
