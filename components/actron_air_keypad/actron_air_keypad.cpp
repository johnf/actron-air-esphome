// #include <esphome/core/hal.h>
#include "actron_air_keypad.h"
#include "esphome/core/gpio.h"
#include <esphome/core/log.h>

namespace esphome {
namespace actron_air_keypad {

static const char *const TAG = "actron_air_keypad";

void IRAM_ATTR ActronAirKeypad::handle_interrupt(ActronAirKeypad *arg) {
  ledProto.handle_interrupt();
}

void ActronAirKeypad::setup() {
  ESP_LOGD(TAG, "Setting up on pin");
  pin_->setup();
  // this->pin_->digital_write(true);  // pull-up

  auto *internal_pin = static_cast<InternalGPIOPin *>(pin_);

  ESP_LOGD(TAG, "Setting up interrupt");
  internal_pin->attach_interrupt(handle_interrupt, this,
                                 gpio::INTERRUPT_FALLING_EDGE);
}

void ActronAirKeypad::loop() {
  ESP_LOGD(TAG, "Looping");
  ledProto.mloop();

  if (ledProto.newdata) {
    ESP_LOGD(TAG, "New data available");
    std::string text;

    for (int i = 0; i < NPULSE; ++i) {
      text += (ledProto.p[i] ? '1' : '0');
    }

    bit_string_->publish_state(text);

    float display_value = ledProto.get_display_value();

    setpoint_temp_->publish_state(display_value);
    this->bit_count_->publish_state(ledProto.dbg_nerr);
    this->room_->publish_state(ledProto.p[LedProtocol::ROOM] != 0);
    this->fan_cont_->publish_state(ledProto.p[LedProtocol::FAN_CONT] != 0);
    this->fan_high_->publish_state(ledProto.p[LedProtocol::FAN_HIGH] != 0);
    this->fan_mid_->publish_state(ledProto.p[LedProtocol::FAN_MID] != 0);
    this->fan_low_->publish_state(ledProto.p[LedProtocol::FAN_LOW] != 0);
    this->cool_->publish_state(ledProto.p[LedProtocol::COOL] != 0);
    this->auto_mode_->publish_state(ledProto.p[LedProtocol::AUTO_MODE] != 0);
    this->heat_->publish_state(ledProto.p[LedProtocol::HEAT] != 0);
    this->run_->publish_state(ledProto.p[LedProtocol::RUN] != 0);
    this->timer_->publish_state(ledProto.p[LedProtocol::TIMER] != 0);
    // this->filter_->publish_state(ledProto.p[LedProtocol::FILTER] != 0);
    this->zone1_->publish_state(ledProto.p[LedProtocol::ZONE1] != 0);
    this->zone2_->publish_state(ledProto.p[LedProtocol::ZONE2] != 0);
    this->zone3_->publish_state(ledProto.p[LedProtocol::ZONE3] != 0);
    this->zone4_->publish_state(ledProto.p[LedProtocol::ZONE4] != 0);
    this->zone5_->publish_state(ledProto.p[LedProtocol::ZONE5] != 0);
    this->zone6_->publish_state(ledProto.p[LedProtocol::ZONE6] != 0);
    this->zone7_->publish_state(ledProto.p[LedProtocol::ZONE7] != 0);
    // this->zone8_->publish_state(ledProto.p[LedProtocol::ZONE8] != 0);

    ledProto.newdata = false;
  }
}

void ActronAirKeypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron Air Keypad:");
  LOG_PIN("  Pin: ", this->pin_);
}

} // namespace actron_air_keypad
} // namespace esphome
