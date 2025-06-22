import esphome.codegen as cg
import esphome.config_validation as cv
import re
from esphome import automation
from esphome.components import cover
from esphome.const import (
    CONF_CLOSE_DURATION,
    CONF_ID,
    CONF_OPEN_DURATION
)
from . import WaremaCoverHub

# Import the rc-switch library
#cg.add_library("rc-switch", "2.6.4")
CONF_HUB_ID = 'warema_cover_hub_id'

CONF_DEVICECODE1 = "devicecode1"
CONF_DEVICECODE2 = "devicecode2"
CONF_CHANNELCODE = "channelcode"

warema_cover_hub_ns = cg.esphome_ns.namespace("warema_cover_hub")
WaremaCover = warema_cover_hub_ns.class_("WaremaCover", cover.Cover, cg.Component)

def validate_binary_string(value, length):
    value = cv.string(value)
    if not re.fullmatch(rf"^[01]{{{length}}}$", value):
        raise cv.Invalid(f"Value must be exactly {length} characters long and contain only 0 or 1.")
    return value

def validate_devicecode(value):
    return validate_binary_string(value, 9)

def validate_channelcode(value):
    return validate_binary_string(value, 7)

CONFIG_SCHEMA = (cover.cover_schema(WaremaCover)
        .extend(
        {
            cv.Required(CONF_HUB_ID): cv.use_id(WaremaCoverHub),  # Pflichtfeld gemacht
            cv.Required(CONF_OPEN_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_CLOSE_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_DEVICECODE1): validate_devicecode,
            cv.Required(CONF_DEVICECODE2): validate_devicecode,
            cv.Required(CONF_CHANNELCODE): validate_channelcode,
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_HUB_ID])  # Replace yield with await
    var = cg.new_Pvariable(config[CONF_ID], hub)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    cg.add(hub.register_cover(var))

    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_devicecode1(config[CONF_DEVICECODE1]))
    cg.add(var.set_devicecode2(config[CONF_DEVICECODE2]))
    cg.add(var.set_channelcode(config[CONF_CHANNELCODE]))
