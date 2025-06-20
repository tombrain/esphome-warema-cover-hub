import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

AUTO_LOAD = ['text_sensor']
MULTI_CONF = True

CONF_HUB_ID = 'warema_cover_hub_id'

warema_cover_hub_ns = cg.esphome_ns.namespace('warema_cover_hub')

WaremaCoverHub = warema_cover_hub_ns.class_('WaremaCoverHub', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WaremaCoverHub),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)