# Actron Air ESP32 Keypad Component for ESPHome

An ESPHome external component for reading and decoding pulse trains from the
Actron Air ESP32 keypad display.

## 🔧 Key Features

- **Interrupt-driven reading**: Accurate pulse train capture
- **40-bit decoding**: Full status from keypad display
- **Temperature display**: 7-segment decoder for setpoint
- **18 binary sensors**: Mode, fan, zones, timers
- **Error tracking**: Bit count for monitoring reliability
- **Type-safe config**: ESPHome validation for all settings

## 🔌 Hardware Requirements

- ESP32 (ESP-IDF framework)
- GPIO pin for pulse train input
- Connection to air conditioner keypad display output

See the forum thread at
<https://community.home-assistant.io/t/actron-aircon-esp32-controller-help/609062>
to build the hardware.

## 🚀 Installation

### Method 1: Local Component (Recommended for Development)

Place the `components` directory in your ESPHome configuration directory:

```text
esphome/
  components/
    actron_air_keypad/
      __init__.py
      keypad.h
      keypad.cpp
      sensor.py
      binary_sensor.py
      text_sensor.py
  your-device.yaml
```

Then in your YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
```

### Method 2: Git Repository

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/johnf/esphome-actron-air-keypad
      ref: main
    components: [actron_air_keypad]
```

## 📝 Basic Configuration

See `example_actron_air_keypad.yaml` for complete configuration with all
sensors and DAC controls.

```yaml
# Load the component
external_components:
  - source:
      type: local
      path: components

# Configure the reader
actron_air_keypad:
  adc_pin: GPIO33

# Add temperature sensor
sensor:
  - platform: actron_air_keypad
    setpoint_temp:
      name: "Temperature"

# Add mode sensors
binary_sensor:
  - platform: actron_air_keypad
    cool:
      name: "Cool Mode"
    heat:
      name: "Heat Mode"
    fan_hi:
      name: "Fan High"
    zone1:
      name: "Zone Bedrooms"
      icon: mdi:bed
    zone2:
      name: "Zone Living"
      icon: mdi:sofa

text_sensor:
  - platform: template
    name: "Fan Mode"
    lambda: |-
      if (id(fan_hi).state && id(fan_cont).state)
        return {"High Continuous"};
      else if (id(fan_hi).state)
        return {"High"};
      else if (id(fan_mid).state)
        return {"Medium"};
      return {"Low"};
```

## 📊 Available Sensors

### Numeric Sensors (`sensor` platform)

- `setpoint_temp` - Temperature from 7-segment display (°C)
- `bit_count` - Error counter for monitoring

### Status Sensors (`binary_sensor` platform)

- **Mode**: `cool`, `heat`, `auto`, `run`
- **Fan**: `fan_hi`, `fan_mid`, `fan_low`, `fan_cont`
- **Zones**: `zone1` through `zone8`
- **Other**: `inside`, `timer`

### Debug Sensors (`text_sensor` platform)

- `bit_string` - Raw 40-bit pulse train (internal use)

## How It Works

1. An interrupt handler captures falling edges on the ADC pin
2. Timing between edges determines if the pulse represents a '0' or '1'
3. 40 bits are collected to form a complete frame
4. The frame is decoded to extract display and LED status
5. Changes are published to Home Assistant sensors

## 🐛 Troubleshooting

### No data received

- Check ADC pin connection
- Verify pulse train voltage levels
- Monitor `bit_count` sensor for errors

### Incorrect temperature readings

- Check 7-segment wiring/decoding
- Look at `bit_string` for raw data
- Verify display is showing temperature

### Compile errors

- Ensure ESP-IDF framework is selected
- Check all Python files are present
- Validate YAML indentation

---

**Need Help?**

- Check ESPHome logs: `esphome logs your-device.yaml`
- Validate config: `esphome config your-device.yaml`
- Review example.yaml for complete setup

## 📄 License

MIT License - Feel free to use and modify
