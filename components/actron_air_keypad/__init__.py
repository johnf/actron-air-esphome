"""Actron Air Keypad ESPHome component.

Decodes the pulse train from an Actron Air keypad display to extract
temperature setpoint, mode indicators, fan speeds, and zone states.
"""

from typing import Any

from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN
from esphome.cpp_helpers import gpio_pin_expression

CODEOWNERS = ['@johnf']
AUTO_LOAD = ['sensor', 'binary_sensor', 'text_sensor']

# Shared constant for referencing the parent component
CONF_ACTRON_AIR_KEYPAD_ID = 'actron_air_keypad_id'

actron_air_keypad_ns = cg.esphome_ns.namespace('actron_air_keypad')
ActronAirKeypad = actron_air_keypad_ns.class_('ActronAirKeypad', cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ActronAirKeypad),
        cv.Required(CONF_PIN): pins.gpio_input_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config: dict[str, Any]) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin = await gpio_pin_expression(config[CONF_PIN])
    if pin is not None:
        cg.add(var.set_pin(pin))
