"""Text sensor platform for Actron Air Keypad component."""

from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import ActronAirKeypad, CONF_ACTRON_AIR_KEYPAD_ID

DEPENDENCIES = ['actron_air_keypad']

CONF_BIT_STRING = 'bit_string'

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ACTRON_AIR_KEYPAD_ID): cv.use_id(ActronAirKeypad),
        cv.Optional(CONF_BIT_STRING): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config: dict[str, Any]) -> None:
    parent = await cg.get_variable(config[CONF_ACTRON_AIR_KEYPAD_ID])

    if CONF_BIT_STRING in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BIT_STRING])
        cg.add(parent.set_bit_string_sensor(sens))
