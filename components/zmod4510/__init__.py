import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

# Optionally, define your own unit constant.
UNIT_PPB = "ppb"

DEPENDENCIES = ['i2c']
AUTO_LOAD = ["sensor"]

zmod4510_ns = cg.global_ns.namespace("zmod4510")
ZMOD4510 = zmod4510_ns.class_("ZMOD4510", i2c.I2CDevice, cg.Component)

CONF_NO2 = "no2"
CONF_O3 = "o3"
CONF_AQI = "aqi"
CONF_ADDRESS = "address"

# Use sensor's schema if you want to attach sensors.
from esphome.components import sensor

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ZMOD4510),
    cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.time_period,
    cv.Optional(CONF_ADDRESS, default=0x33): cv.hex_int,
    cv.Optional(CONF_NO2): sensor.sensor_schema(),
    cv.Optional(CONF_O3): sensor.sensor_schema(),
    cv.Optional(CONF_AQI): sensor.sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA).extend(
    cv.polling_component_schema("60s").extend(i2c.i2c_device_schema(0x33))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL].total_milliseconds()))
    cg.add(var.set_i2c_address(config[CONF_ADDRESS]))
    if CONF_NO2 in config:
        no2_sensor = await sensor.new_sensor(config[CONF_NO2])
        cg.add(var.set_no2_sensor(no2_sensor))
    if CONF_O3 in config:
        o3_sensor = await sensor.new_sensor(config[CONF_O3])
        cg.add(var.set_o3_sensor(o3_sensor))
    if CONF_AQI in config:
        aqi_sensor = await sensor.new_sensor(config[CONF_AQI])
        cg.add(var.set_aqi_sensor(aqi_sensor))
