"""Climate platform for Actron Air ESPHome integration."""
from __future__ import annotations

import asyncio
import logging
from typing import Any

from homeassistant.components.climate import (
    ClimateEntity,
    ClimateEntityFeature,
    HVACAction,
    HVACMode,
)
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import ATTR_TEMPERATURE, UnitOfTemperature
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.event import async_track_state_change_event

from .const import (
    CONF_CURRENT_TEMP_SENSOR,
    CONF_ENTITY_PREFIX,
    CONF_HUMIDITY_SENSOR,
    CONF_ZONE_COUNT,
    DEFAULT_ZONE_COUNT,
    DOMAIN,
    ENTITY_SUFFIXES,
    FAN_MODE_HIGH,
    FAN_MODE_HIGH_CONT,
    FAN_MODE_LOW,
    FAN_MODE_LOW_CONT,
    FAN_MODE_MEDIUM,
    FAN_MODE_MEDIUM_CONT,
    FAN_MODE_OFF,
    FAN_MODES,
    MAX_TEMP,
    MIN_TEMP,
    PRESET_ALL_ZONES,
    PRESET_ZONE_PREFIX,
    TEMP_STEP,
)

_LOGGER = logging.getLogger(__name__)

# Delay between button presses (ms)
BUTTON_PRESS_DELAY = 0.5


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up Actron Air ESPHome climate from a config entry."""
    entity_prefix = entry.data[CONF_ENTITY_PREFIX]
    zone_count = int(entry.data.get(CONF_ZONE_COUNT, DEFAULT_ZONE_COUNT))
    current_temp_sensor = entry.data.get(CONF_CURRENT_TEMP_SENSOR)
    humidity_sensor = entry.data.get(CONF_HUMIDITY_SENSOR)

    async_add_entities(
        [
            ActronAirClimate(
                hass,
                entry,
                entity_prefix,
                zone_count,
                current_temp_sensor,
                humidity_sensor,
            )
        ]
    )


class ActronAirClimate(ClimateEntity):
    """Actron Air ESPHome Climate Entity."""

    _attr_has_entity_name = True
    _attr_name = "Climate"
    _attr_temperature_unit = UnitOfTemperature.CELSIUS
    _attr_target_temperature_step = TEMP_STEP
    _attr_min_temp = MIN_TEMP
    _attr_max_temp = MAX_TEMP
    _attr_hvac_modes = [
        HVACMode.OFF,
        HVACMode.COOL,
        HVACMode.HEAT,
        HVACMode.AUTO,
        HVACMode.FAN_ONLY,
    ]
    _attr_fan_modes = FAN_MODES
    _attr_supported_features = (
        ClimateEntityFeature.TARGET_TEMPERATURE
        | ClimateEntityFeature.FAN_MODE
        | ClimateEntityFeature.PRESET_MODE
        | ClimateEntityFeature.TURN_ON
        | ClimateEntityFeature.TURN_OFF
    )

    def __init__(
        self,
        hass: HomeAssistant,
        entry: ConfigEntry,
        entity_prefix: str,
        zone_count: int,
        current_temp_sensor: str | None,
        humidity_sensor: str | None,
    ) -> None:
        """Initialise the climate entity."""
        self.hass = hass
        self._entry = entry
        self._entity_prefix = entity_prefix
        self._zone_count = zone_count
        self._current_temp_sensor = current_temp_sensor
        self._humidity_sensor = humidity_sensor
        self._attr_unique_id = f"{entity_prefix}_climate"

        # Build preset modes list
        self._attr_preset_modes = self._build_preset_modes()

        # Entity IDs for state tracking
        self._temp_entity = (
            f"sensor.{entity_prefix}_{ENTITY_SUFFIXES['setpoint_temp']}"
        )
        self._power_entity = f"switch.{entity_prefix}_{ENTITY_SUFFIXES['power']}"

    def _build_preset_modes(self) -> list[str]:
        """Build the list of preset modes based on zone count."""
        presets = []
        for i in range(1, self._zone_count + 1):
            presets.append(f"{PRESET_ZONE_PREFIX}{i}")
        if self._zone_count > 1:
            presets.append(PRESET_ALL_ZONES)

        return presets

    def _get_entity_id(self, entity_type: str, suffix_key: str) -> str:
        """Get the full entity ID for a given suffix key."""
        return f"{entity_type}.{self._entity_prefix}_{ENTITY_SUFFIXES[suffix_key]}"

    def _get_zone_entity_id(self, zone_num: int, entity_type: str = "binary_sensor") -> str:
        """Get the entity ID for a zone."""
        return f"{entity_type}.{self._entity_prefix}_zone_{zone_num}"

    def _get_zone_switch_id(self, zone_num: int) -> str:
        """Get the switch entity ID for a zone."""
        return f"switch.{self._entity_prefix}_zone_{zone_num}"

    async def async_added_to_hass(self) -> None:
        """Run when entity is added."""
        await super().async_added_to_hass()

        # Build list of entities to track
        entities_to_track = [
            self._temp_entity,
            self._power_entity,
            self._get_entity_id("binary_sensor", "cool"),
            self._get_entity_id("binary_sensor", "heat"),
            self._get_entity_id("binary_sensor", "auto_mode"),
            self._get_entity_id("binary_sensor", "run"),
            self._get_entity_id("binary_sensor", "fan_low"),
            self._get_entity_id("binary_sensor", "fan_mid"),
            self._get_entity_id("binary_sensor", "fan_high"),
            self._get_entity_id("binary_sensor", "fan_cont"),
        ]

        # Add zone entities
        for i in range(1, self._zone_count + 1):
            entities_to_track.append(self._get_zone_entity_id(i))

        # Add optional external sensors
        if self._current_temp_sensor:
            entities_to_track.append(self._current_temp_sensor)
        if self._humidity_sensor:
            entities_to_track.append(self._humidity_sensor)

        self.async_on_remove(
            async_track_state_change_event(
                self.hass, entities_to_track, self._async_state_changed
            )
        )

    @callback
    def _async_state_changed(self, event) -> None:
        """Handle state changes of tracked entities."""
        self.async_write_ha_state()

    def _is_binary_sensor_on(self, suffix_key: str) -> bool:
        """Check if a binary sensor is on."""
        entity_id = self._get_entity_id("binary_sensor", suffix_key)
        state = self.hass.states.get(entity_id)

        return state is not None and state.state == "on"

    def _is_zone_on(self, zone_num: int) -> bool:
        """Check if a zone is on."""
        entity_id = self._get_zone_entity_id(zone_num)
        state = self.hass.states.get(entity_id)

        return state is not None and state.state == "on"

    def _get_sensor_value(self, entity_id: str) -> float | None:
        """Get a numeric value from a sensor."""
        state = self.hass.states.get(entity_id)
        if state is None or state.state in ("unknown", "unavailable"):
            return None
        try:
            return float(state.state)
        except ValueError:
            return None

    @property
    def target_temperature(self) -> float | None:
        """Return the temperature we try to reach."""
        return self._get_sensor_value(self._temp_entity)

    @property
    def current_temperature(self) -> float | None:
        """Return the current temperature."""
        if self._current_temp_sensor:
            return self._get_sensor_value(self._current_temp_sensor)

        return None

    @property
    def current_humidity(self) -> int | None:
        """Return the current humidity."""
        if self._humidity_sensor:
            value = self._get_sensor_value(self._humidity_sensor)
            if value is not None:
                return int(value)

        return None

    @property
    def hvac_mode(self) -> HVACMode:
        """Return current HVAC mode."""
        # Check power state first
        power_state = self.hass.states.get(self._power_entity)
        if power_state is None or power_state.state != "on":
            return HVACMode.OFF

        if self._is_binary_sensor_on("heat"):
            return HVACMode.HEAT
        elif self._is_binary_sensor_on("cool"):
            return HVACMode.COOL
        elif self._is_binary_sensor_on("auto_mode"):
            return HVACMode.AUTO

        # Check for fan-only mode (fan running but no heat/cool/auto)
        fan_on = (
            self._is_binary_sensor_on("fan_high")
            or self._is_binary_sensor_on("fan_mid")
            or self._is_binary_sensor_on("fan_low")
        )
        if fan_on:
            return HVACMode.FAN_ONLY

        return HVACMode.OFF

    @property
    def hvac_action(self) -> HVACAction | None:
        """Return the current running HVAC action."""
        hvac_mode = self.hvac_mode
        is_running = self._is_binary_sensor_on("run")

        if hvac_mode == HVACMode.OFF:
            return HVACAction.OFF

        if hvac_mode == HVACMode.HEAT:
            return HVACAction.HEATING if is_running else HVACAction.IDLE

        if hvac_mode == HVACMode.COOL:
            return HVACAction.COOLING if is_running else HVACAction.IDLE

        if hvac_mode == HVACMode.AUTO:
            if is_running:
                # Could be heating or cooling in auto mode
                return HVACAction.COOLING  # Default to cooling for auto
            return HVACAction.IDLE

        if hvac_mode == HVACMode.FAN_ONLY:
            return HVACAction.FAN

        return None

    @property
    def fan_mode(self) -> str | None:
        """Return the fan setting."""
        fan_cont = self._is_binary_sensor_on("fan_cont")

        if self._is_binary_sensor_on("fan_high"):
            return FAN_MODE_HIGH_CONT if fan_cont else FAN_MODE_HIGH
        elif self._is_binary_sensor_on("fan_mid"):
            return FAN_MODE_MEDIUM_CONT if fan_cont else FAN_MODE_MEDIUM
        elif self._is_binary_sensor_on("fan_low"):
            return FAN_MODE_LOW_CONT if fan_cont else FAN_MODE_LOW

        return FAN_MODE_OFF

    @property
    def preset_mode(self) -> str | None:
        """Return the current preset mode based on active zones."""
        active_zones = []
        for i in range(1, self._zone_count + 1):
            if self._is_zone_on(i):
                active_zones.append(i)

        if len(active_zones) == 0:
            return None
        elif len(active_zones) == self._zone_count:
            return PRESET_ALL_ZONES
        elif len(active_zones) == 1:
            return f"{PRESET_ZONE_PREFIX}{active_zones[0]}"

        # Multiple but not all zones - return first active
        return f"{PRESET_ZONE_PREFIX}{active_zones[0]}"

    async def _press_button(self, suffix_key: str) -> None:
        """Press a button entity."""
        button_entity = self._get_entity_id("button", suffix_key)
        await self.hass.services.async_call(
            "button", "press", {"entity_id": button_entity}
        )

    async def _set_switch(self, entity_id: str, turn_on: bool) -> None:
        """Set a switch state."""
        service = "turn_on" if turn_on else "turn_off"
        await self.hass.services.async_call(
            "switch", service, {"entity_id": entity_id}
        )

    async def async_set_temperature(self, **kwargs: Any) -> None:
        """Set new target temperature."""
        target_temp = kwargs.get(ATTR_TEMPERATURE)
        if target_temp is None:
            return

        current_temp = self.target_temperature
        if current_temp is None:
            _LOGGER.warning("Cannot set temperature: current setpoint unknown")

            return

        # Calculate how many button presses needed
        diff = target_temp - current_temp
        steps = int(abs(diff) / TEMP_STEP)

        if steps == 0:
            return

        suffix_key = "temp_up" if diff > 0 else "temp_down"

        for _ in range(steps):
            await self._press_button(suffix_key)
            await asyncio.sleep(BUTTON_PRESS_DELAY)

    async def async_set_hvac_mode(self, hvac_mode: HVACMode) -> None:
        """Set new target HVAC mode."""
        current_mode = self.hvac_mode

        # Target is OFF - just turn it off
        if hvac_mode == HVACMode.OFF:
            if current_mode != HVACMode.OFF:
                await self._set_switch(self._power_entity, False)

            return

        # Target is FAN_ONLY - turn off power then press fan button
        if hvac_mode == HVACMode.FAN_ONLY:
            if current_mode != HVACMode.OFF:
                await self._set_switch(self._power_entity, False)
                await asyncio.sleep(BUTTON_PRESS_DELAY)

            await self._press_button("fan_button")

            return

        # Target is a mode (cool/heat/auto)
        # Turn on first if currently off
        if current_mode == HVACMode.OFF:
            await self._set_switch(self._power_entity, True)
            await asyncio.sleep(BUTTON_PRESS_DELAY)

        # Calculate presses needed using modular arithmetic
        # Mode order: cool -> heat -> auto -> cool...
        modes = [HVACMode.COOL, HVACMode.HEAT, HVACMode.AUTO]
        mode_sensors = {
            HVACMode.COOL: "cool",
            HVACMode.HEAT: "heat",
            HVACMode.AUTO: "auto_mode",
        }

        # Determine current mode from binary sensors
        current_mode_idx = 0
        for idx, mode in enumerate(modes):
            if self._is_binary_sensor_on(mode_sensors[mode]):
                current_mode_idx = idx
                break

        target_mode_idx = modes.index(hvac_mode) if hvac_mode in modes else 0
        presses_needed = (target_mode_idx - current_mode_idx) % 3

        for _ in range(presses_needed):
            await self._press_button("mode_button")
            await asyncio.sleep(BUTTON_PRESS_DELAY)

    async def async_set_fan_mode(self, fan_mode: str) -> None:
        """Set new target fan mode."""
        current_fan = self.fan_mode

        if current_fan == fan_mode:
            return

        # Fan mode order for cycling
        fan_modes_order = [
            FAN_MODE_HIGH,
            FAN_MODE_MEDIUM,
            FAN_MODE_LOW,
            FAN_MODE_HIGH_CONT,
            FAN_MODE_MEDIUM_CONT,
            FAN_MODE_LOW_CONT,
        ]

        # Calculate presses needed using modular arithmetic
        if current_fan in fan_modes_order and fan_mode in fan_modes_order:
            current_idx = fan_modes_order.index(current_fan)
            target_idx = fan_modes_order.index(fan_mode)
            presses_needed = (target_idx - current_idx) % 6
        else:
            # Fallback: press up to 6 times
            presses_needed = 6

        for _ in range(presses_needed):
            await self._press_button("fan_button")
            await asyncio.sleep(BUTTON_PRESS_DELAY)

    async def async_set_preset_mode(self, preset_mode: str) -> None:
        """Set new preset mode (zone configuration)."""
        if preset_mode == PRESET_ALL_ZONES:
            # Turn on all zones
            for i in range(1, self._zone_count + 1):
                if not self._is_zone_on(i):
                    await self._set_switch(self._get_zone_switch_id(i), True)
                    await asyncio.sleep(BUTTON_PRESS_DELAY)
        elif preset_mode.startswith(PRESET_ZONE_PREFIX):
            # Single zone preset
            try:
                target_zone = int(preset_mode.replace(PRESET_ZONE_PREFIX, ""))
            except ValueError:
                _LOGGER.error("Invalid preset mode: %s", preset_mode)

                return

            # Turn on target zone, turn off others
            for i in range(1, self._zone_count + 1):
                should_be_on = i == target_zone
                is_on = self._is_zone_on(i)

                if should_be_on != is_on:
                    await self._set_switch(self._get_zone_switch_id(i), should_be_on)
                    await asyncio.sleep(BUTTON_PRESS_DELAY)

    async def async_turn_on(self) -> None:
        """Turn the entity on."""
        await self._set_switch(self._power_entity, True)

    async def async_turn_off(self) -> None:
        """Turn the entity off."""
        await self._set_switch(self._power_entity, False)

    @property
    def device_info(self) -> DeviceInfo:
        """Return device info."""
        return DeviceInfo(
            identifiers={(DOMAIN, self._entity_prefix)},
            name=f"Actron Air ESPHome ({self._entity_prefix})",
            manufacturer="Actron Air",
            model="ESPHome Keypad Controller",
        )
