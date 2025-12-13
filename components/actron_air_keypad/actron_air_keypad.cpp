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

  internal_pin->attach_interrupt(handle_interrupt, this,
                                 gpio::INTERRUPT_FALLING_EDGE);
}

void ActronAirKeypad::loop() {
  ledProto.mloop();

  if (ledProto.newdata) {
    std::string text;
    for (int i = 0; i < NPULSE; ++i) {
      text += (ledProto.p[i] ? '1' : '0');
    }

    if (this->bit_string_ != nullptr) {
      this->bit_string_->publish_state(text);
    }

    float display_value = ledProto.get_display_value();
    if (this->setpoint_temp_ != nullptr) {
      this->setpoint_temp_->publish_state(display_value);
    }
    if (this->bit_count_ != nullptr)
      this->bit_count_->publish_state(ledProto.dbg_nerr);

    if (this->room_ != nullptr) {
      this->room_->publish_state(ledProto.p[LedProtocol::ROOM] != 0);
    }
    if (this->fan_cont_ != nullptr) {
      this->fan_cont_->publish_state(ledProto.p[LedProtocol::FAN_CONT] != 0);
    }
    if (this->fan_high_ != nullptr) {
      this->fan_high_->publish_state(ledProto.p[LedProtocol::FAN_HIGH] != 0);
    }
    if (this->fan_mid_ != nullptr) {
      this->fan_mid_->publish_state(ledProto.p[LedProtocol::FAN_MID] != 0);
    }
    if (this->fan_low_ != nullptr) {
      this->fan_low_->publish_state(ledProto.p[LedProtocol::FAN_LOW] != 0);
    }
    if (this->cool_ != nullptr) {
      this->cool_->publish_state(ledProto.p[LedProtocol::COOL] != 0);
    }
    if (this->auto_ != nullptr) {
      this->auto_->publish_state(ledProto.p[LedProtocol::AUTO] != 0);
    }
    if (this->heat_ != nullptr) {
      this->heat_->publish_state(ledProto.p[LedProtocol::HEAT] != 0);
    }
    if (this->run_ != nullptr) {
      this->run_->publish_state(ledProto.p[LedProtocol::RUN] != 0);
    }
    if (this->timer_ != nullptr) {
      this->timer_->publish_state(ledProto.p[LedProtocol::TIMER] != 0);
    }
    // if (this->filter_ != nullptr) {
    //   this->filter_->publish_state(ledProto.p[LedProtocol::FILTER] != 0);
    // }
    if (this->zone1_ != nullptr) {
      this->zone1_->publish_state(ledProto.p[LedProtocol::ZONE1] != 0);
    }
    if (this->zone2_ != nullptr) {
      this->zone2_->publish_state(ledProto.p[LedProtocol::ZONE2] != 0);
    }
    if (this->zone3_ != nullptr) {
      this->zone3_->publish_state(ledProto.p[LedProtocol::ZONE3] != 0);
    }
    if (this->zone4_ != nullptr) {
      this->zone4_->publish_state(ledProto.p[LedProtocol::ZONE4] != 0);
    }
    if (this->zone5_ != nullptr) {
      this->zone5_->publish_state(ledProto.p[LedProtocol::ZONE5] != 0);
    }
    if (this->zone6_ != nullptr) {
      this->zone6_->publish_state(ledProto.p[LedProtocol::ZONE6] != 0);
    }
    if (this->zone7_ != nullptr) {
      this->zone7_->publish_state(ledProto.p[LedProtocol::ZONE7] != 0);
    }
    // if (this->zone8_ != nullptr) {
    //   this->zone8_->publish_state(ledProto.p[LedProtocol::ZONE8] != 0);
    // }

    ledProto.newdata = false;
  }
}

void ActronAirKeypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron Air Keypad:");
  LOG_PIN("  Pin: ", this->pin_);
}

} // namespace actron_air_keypad
} // namespace esphome
