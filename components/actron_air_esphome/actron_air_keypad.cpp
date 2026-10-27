#include "actron_air_keypad.h"

#include <cmath>

namespace esphome {
namespace actron_air_esphome {

static const char *const TAG = "actron_air_esphome";

// Mapping from BinarySensorId to LedIndex with inversion flag
struct BinarySensorMapping {
  LedIndex led_index;
  bool invert;
};

// Static mapping table - order must match BinarySensorId enum
static constexpr std::array<BinarySensorMapping, BINARY_SENSOR_COUNT>
    BINARY_SENSOR_MAPPINGS = {{
        {LedIndex::FAN_CONT, false},
        {LedIndex::FAN_HIGH, false},
        {LedIndex::FAN_MID, false},
        {LedIndex::FAN_LOW, false},
        {LedIndex::COOL, false},
        {LedIndex::AUTO_MODE, false},
        {LedIndex::HEAT, false},
        {LedIndex::RUN, false},
        {LedIndex::TIMER, false},
        {LedIndex::INSIDE, false},
        {LedIndex::ZONE_1, false},
        {LedIndex::ZONE_2, false},
        {LedIndex::ZONE_3, false},
        {LedIndex::ZONE_4, false},
        {LedIndex::ZONE_5, true}, // Zones 5-8 have inverted logic
        {LedIndex::ZONE_6, true},
        {LedIndex::ZONE_7, true},
        {LedIndex::ZONE_8, true},
    }};

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
  case 0x5F:
    return 'S'; // AFGCD

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
    return NAN;
  }

  int value = (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0');
  float display_value = static_cast<float>(value);

  if (get_pulse(L::DP)) {
    display_value *= 0.1f;
  }

  if (display_value < SETPOINT_MIN_C || display_value > SETPOINT_MAX_C) {
    return NAN;
  }

  return display_value;
}

void IRAM_ATTR ActronAirKeypad::handle_interrupt(ActronAirKeypad *arg) {
  switch (arg->decoder_.on_edge(micros())) {
  case FrameDecoder::Result::FRAME:
    arg->frame_bits_.store(arg->decoder_.frame(), std::memory_order_relaxed);
    arg->frame_ready_.store(true, std::memory_order_release);
    break;
  case FrameDecoder::Result::ERROR:
    arg->error_count_.fetch_add(1, std::memory_order_relaxed);
    break;
  case FrameDecoder::Result::NONE:
    break;
  }
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
  uint32_t errors = error_count_.load(std::memory_order_relaxed);
  if (error_count_sensor_ && errors != published_error_count_) {
    published_error_count_ = errors;
    error_count_sensor_->publish_state(errors);
  }

  if (!frame_ready_.exchange(false, std::memory_order_acquire)) {
    return;
  }

  uint64_t bits = frame_bits_.load(std::memory_order_relaxed);
  if (!confirmer_.accept(bits)) {
    return;
  }

  pulses_ = bits;
  ESP_LOGD(TAG, "New data available");

  if (bit_string_) {
    std::string text;
    text.reserve(NPULSE);
    for (std::size_t i = 0; i < NPULSE; ++i) {
      text += ((pulses_ >> i) & 1) ? '1' : '0';
    }
    bit_string_->publish_state(text);
  }

  if (setpoint_temp_) {
    float setpoint = get_display_value();
    if (!std::isnan(setpoint)) {
      setpoint_temp_->publish_state(setpoint);
    }
  }

  for (std::size_t i = 0; i < BINARY_SENSOR_COUNT; ++i) {
    if (binary_sensors_[i]) {
      const auto &mapping = BINARY_SENSOR_MAPPINGS[i];
      bool state = get_pulse(mapping.led_index);
      binary_sensors_[i]->publish_state(mapping.invert ? !state : state);
    }
  }
}

void ActronAirKeypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron Air ESPHome:");
  LOG_PIN("  Pin: ", this->pin_);
}

} // namespace actron_air_esphome
} // namespace esphome
