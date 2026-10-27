# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESPHome external component that decodes pulse trains from Actron Air keypad displays. Captures 40-bit frames via GPIO interrupts and exposes temperature setpoint, mode indicators, fan speeds, and zone states as Home Assistant sensors. Also includes a Home Assistant integration with climate entity and custom Lovelace card.

## Architecture

### ESPHome Component Structure

```
components/actron_air_esphome/
├── __init__.py           # ESPHome component registration and config schema
├── actron_air_keypad.h   # Main component class (protocol, timing, enums)
├── actron_air_keypad.cpp # Component implementation (ISR, decoding, publishing)
├── frame_decoder.h       # Edge-to-frame decoder and frame confirmation (host-testable)
├── sensor.py             # Numeric sensor platform (setpoint_temp, error_count)
├── binary_sensor.py      # Binary sensor platform (18 sensors: modes, fans, zones)
└── text_sensor.py        # Text sensor platform (bit_string for debugging)
```

### Home Assistant Integration Structure

```
custom_components/actron_air_esphome/
├── __init__.py           # Integration setup, frontend card registration
├── climate.py            # Climate entity implementation
├── config_flow.py        # UI-based configuration
├── const.py              # Constants and entity mappings
├── manifest.json         # Integration metadata
├── strings.json          # Config flow UI strings
├── translations/
│   └── en.json           # English translations
└── frontend/
    └── actron-air-esphome-card.js  # Bundled Lovelace card
```

### Lovelace Card Structure

```
src/
├── index.ts    # Main card component
├── editor.ts   # Config editor
├── styles.ts   # Retro LCD styling
├── types.ts    # TypeScript interfaces
└── const.ts    # Constants and mappings
```

### Data Flow

```
GPIO Pin → ISR (handle_interrupt) → Main Loop → Publish to Sensors
```

1. Hardware interrupt captures falling edges on GPIO pin
2. ISR feeds edge times to `FrameDecoder` (frame_decoder.h), which hands complete
   40-bit frames to the main loop through atomics
3. `loop()` passes frames through `FrameConfirmer` and publishes confirmed changes

### Python-C++ Integration

- Python files define ESPHome config schemas and generate C++ code
- `__init__.py` auto-loads sensor, binary_sensor, and text_sensor platforms
- `to_code()` functions use `cg.new_Pvariable()` and setter methods to wire sensors

### Protocol Details

Bits are encoded in the gap between falling edges; each edge starts a ~62us low pulse.

- **Frame size:** 40 bits
- **Bits:** ~490us gap = 1, ~1470us gap = 0 (threshold 1ms)
- **Start gap:** ~2.9ms before the first bit, but only when the timer LED (bit 7) is off
- **Frame boundary:** any gap >= 2.7ms (idle gap between frames is ~200ms, ~4 frames/s)
- **Bounce:** ~1us bounces around edges; a rising-edge bounce adds a falling edge 62us
  after the real one, so edges within 250us of the last accepted edge are ignored
- **Confirmation:** a changed frame is published only after 2 identical frames in a
  row, so blinking displays (timer setting, room temperature) hold the last steady state

### Key Classes

- `ActronAirKeypad` (actron_air_keypad.h) - ESPHome Component handling ISR, protocol decoding, and sensor publishing
- `ActronAirClimate` (climate.py) - Home Assistant climate entity wrapping ESPHome sensors

## Available Sensors

**Numeric (`sensor` platform):** `setpoint_temp`, `error_count`

**Binary (`binary_sensor` platform):** `cool`, `heat`, `auto_mode`, `run`, `fan_low`, `fan_mid`, `fan_high`, `fan_cont`, `zone_1`-`zone_8`, `timer`, `inside`

**Text (`text_sensor` platform):** `bit_string`

## Climate Entity Features

- **HVAC Modes:** off, cool, heat, auto, fan_only
- **Fan Modes:** High, Medium, Low, High Cont, Medium Cont, Low Cont, Off
- **Zone Presets:** Individual zones and "All Zones"
- **Optional:** Current temperature and humidity from external sensors

## Requirements

- ESP32 with ESP-IDF framework
- Python 3.13+
- ESPHome >= 2025.11.5
- Home Assistant >= 2024.1.0 (for integration)
- uv (for Python environment management)

## Development Setup

```bash
# Set up Python environment and install ESPHome
./setup-dev.sh

# Activate the virtual environment
source .venv/bin/activate
```

## Build Commands

### ESPHome Component

```bash
# Test that the ESPHome component compiles (uses local components)
.venv/bin/esphome compile test_compile.yaml

# Validate configuration without compiling
.venv/bin/esphome config test_compile.yaml

# Host tests for the frame decoder, replaying captured keypad waveforms
scripts/test-decoder.sh
```

### Lovelace Card

- `pnpm build` - Runs linting (Biome + TypeScript + Knip) then Vite build, copies output to integration
- `pnpm lint:biome` - Run Biome linter/formatter
- `pnpm lint:types` - TypeScript type checking (`tsc --noEmit`)
- `pnpm lint:knip` - Check for unused exports/dependencies

## Development Tooling

- **Biome** (`biome.json`) - Linting and formatting for TypeScript
- **Knip** (`knip.jsonc`) - Unused code/dependency detection
- **Vite** - Build bundler (config inline via package.json, no separate config file)
- **pnpm** - Package manager
- **docker-compose.yml** - Local Home Assistant instance for testing

## CI/CD & Release

### Workflows

- `.github/workflows/build.yml` - Builds card and compiles ESPHome component
- `.github/workflows/release.yml` - Semantic release
- `.github/workflows/validate.yml` - HACS and hassfest validation

### Release Process

- `release.config.mjs` - semantic-release configuration
- `scripts/update-manifest-version.mjs` - Updates version in manifest.json, package.json, and src/const.ts
- Commit convention: conventional commits (`feat:`, `fix:`, etc.)

## Maintenance

Keep this file updated when making architectural changes to the codebase.
