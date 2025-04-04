import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL


AUTO_LOAD = ["sensor"]
# This ensures that the I2C component is available.
DEPENDENCIES = ['i2c']

# Create a namespace for your component.
zmod4510_ns = cg.global_ns.namespace('zmod4510')
# Declare the main C++ class. It must be defined in your C++ wrapper.
ZMOD4510 = zmod_ns.class_('ZMOD4510', i2c.I2CDevice, cg.Component)

# Define configuration keys.
CONF_NO2 = "no2"
CONF_O3 = "o3"
CONF_AQI = "aqi"
CONF_I2C_ADDRESS = "i2c_address"

# Define the configuration schema.
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ZMOD4510),
    cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.time_period,
    cv.Optional(CONF_I2C_ADDRESS, default=0x33): cv.hex_int,
    cv.Optional(CONF_NO2): sensor.sensor_schema(),
    cv.Optional(CONF_O3): sensor.sensor_schema(),
    cv.Optional(CONF_AQI): sensor.sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s").extend(i2c.i2c_device_schema(0x33)))

# The to_code function converts the YAML configuration into C++ code.
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_polling_component(var, config)
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    cg.add(var.set_i2c_address(config['i2c_address']))
    if 'no2' in config:
        no2_sensor = await sensor.new_sensor(config['no2'])
        cg.add(var.set_no2_sensor(no2_sensor))
    if 'o3' in config:
        o3_sensor = await sensor.new_sensor(config['o3'])
        cg.add(var.set_o3_sensor(o3_sensor))
    if 'aqi' in config:
        aqi_sensor = await sensor.new_sensor(config['aqi'])
        cg.add(var.set_aqi_sensor(aqi_sensor))
