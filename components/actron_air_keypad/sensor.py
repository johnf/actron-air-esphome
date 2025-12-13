import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from . import ActronAirKeypad

DEPENDENCIES = ["actron_air_keypad"]

CONF_KEYPAD_STATUS_ID = "keypad_status_id"
CONF_SETPOINT_TEMP = "setpoint_temp"
CONF_BIT_COUNT = "bit_count"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_KEYPAD_STATUS_ID): cv.use_id(ActronAirKeypad),
        cv.Optional(CONF_SETPOINT_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BIT_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_KEYPAD_STATUS_ID])

    if CONF_SETPOINT_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_SETPOINT_TEMP])
        cg.add(parent.set_setpoint_temp_sensor(sens))

    if CONF_BIT_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_BIT_COUNT])
        cg.add(parent.set_bit_count_sensor(sens))
