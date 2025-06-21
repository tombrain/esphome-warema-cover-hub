import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import WaremaCoverHub, CONF_HUB_ID

DEPENDENCIES = ['text_sensor']

warema_cover_hub_ns = cg.esphome_ns.namespace('warema_cover_hub')
TestTextSensor = warema_cover_hub_ns.class_('TestTextSensor', text_sensor.TextSensor, cg.Component)

CONF_SPRUNGWEITE = 'sprungweite'
CONF_NAME = 'name'  # Add the new property "name"

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(TestTextSensor),
    cv.GenerateID(CONF_HUB_ID): cv.use_id(WaremaCoverHub),
    cv.Optional(CONF_SPRUNGWEITE, default=1): cv.int_range(min=1, max=5),
    cv.Optional(CONF_NAME, default="default_name"): cv.string,  # Add validation for "name"
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    hub = yield cg.get_variable(config[CONF_HUB_ID])
    var = cg.new_Pvariable(config[CONF_ID], hub)
    yield cg.register_component(var, config)
    cg.add(hub.register_text_sensorX(var))
    
    if CONF_SPRUNGWEITE in config:
        cg.add(var.set_sprungweite(config[CONF_SPRUNGWEITE]))
    
    if CONF_NAME in config:  # Set the "name" property
        cg.add(var.set_name(config[CONF_NAME]))