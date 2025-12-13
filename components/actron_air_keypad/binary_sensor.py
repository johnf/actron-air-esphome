import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import binary_sensor

from . import ActronAirKeypad

DEPENDENCIES = ["actron_air_keypad"]

CONF_KEYPAD_STATUS_ID = "keypad_status_id"
CONF_ROOM = "room"
CONF_FAN_CONT = "fan_cont"
CONF_FAN_HIGH = "fan_high"
CONF_FAN_MID = "fan_mid"
CONF_FAN_LOW = "fan_low"
CONF_COOL = "cool"
CONF_AUTO = "auto"
CONF_HEAT = "heat"
CONF_RUN = "run"
CONF_TIMER = "timer"
CONF_FILTER = "filter"
CONF_ZONE1 = "zone1"
CONF_ZONE2 = "zone2"
CONF_ZONE3 = "zone3"
CONF_ZONE4 = "zone4"
CONF_ZONE5 = "zone5"
CONF_ZONE6 = "zone6"
CONF_ZONE7 = "zone7"
CONF_ZONE8 = "zone8"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_KEYPAD_STATUS_ID): cv.use_id(ActronAirKeypad),
        cv.Optional(CONF_ROOM): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FAN_CONT): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FAN_HIGH): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FAN_MID): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FAN_LOW): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_COOL): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_AUTO): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_HEAT): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_RUN): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_TIMER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FILTER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE1): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE2): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE3): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE4): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE5): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE6): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE7): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ZONE8): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_KEYPAD_STATUS_ID])

    if CONF_ROOM in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ROOM])
        cg.add(parent.set_room_sensor(sens))

    if CONF_FAN_CONT in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FAN_CONT])
        cg.add(parent.set_fan_cont_sensor(sens))

    if CONF_FAN_HIGH in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FAN_HIGH])
        cg.add(parent.set_fan_high_sensor(sens))

    if CONF_FAN_MID in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FAN_MID])
        cg.add(parent.set_fan_mid_sensor(sens))

    if CONF_FAN_LOW in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FAN_LOW])
        cg.add(parent.set_fan_low_sensor(sens))

    if CONF_COOL in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_COOL])
        cg.add(parent.set_cool_sensor(sens))

    if CONF_AUTO in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_AUTO])
        cg.add(parent.set_auto_sensor(sens))

    if CONF_HEAT in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_HEAT])
        cg.add(parent.set_heat_sensor(sens))

    if CONF_RUN in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_RUN])
        cg.add(parent.set_run_sensor(sens))

    if CONF_TIMER in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_TIMER])
        cg.add(parent.set_timer_sensor(sens))

    if CONF_FILTER in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FILTER])
        cg.add(parent.set_filter_sensor(sens))

    if CONF_ZONE1 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE1])
        cg.add(parent.set_zone1_sensor(sens))

    if CONF_ZONE2 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE2])
        cg.add(parent.set_zone2_sensor(sens))

    if CONF_ZONE3 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE3])
        cg.add(parent.set_zone3_sensor(sens))

    if CONF_ZONE4 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE4])
        cg.add(parent.set_zone4_sensor(sens))

    if CONF_ZONE5 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE5])
        cg.add(parent.set_zone5_sensor(sens))

    if CONF_ZONE6 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE6])
        cg.add(parent.set_zone6_sensor(sens))

    if CONF_ZONE7 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE7])
        cg.add(parent.set_zone7_sensor(sens))

    if CONF_ZONE8 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ZONE8])
        cg.add(parent.set_zone8_sensor(sens))
