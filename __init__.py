import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NAME
from esphome import pins

# Import the rc-switch library
cg.add_library("rc-switch", "2.6.4")

AUTO_LOAD = ['cover']

CONF_TRANSMITTER_PIN = 'transmitter_pin'
CONF_NUMBER_OF_TRANSMISSIONS = "number_of_transmissions"

warema_cover_hub_ns = cg.esphome_ns.namespace('warema_cover_hub')

WaremaCoverHub = warema_cover_hub_ns.class_('WaremaCoverHub', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WaremaCoverHub),
    cv.Required(CONF_NAME): cv.string,
    cv.Required(CONF_TRANSMITTER_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_NUMBER_OF_TRANSMISSIONS, default=3): cv.int_range(min=3, max=10),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    cg.add(var.set_name(config[CONF_NAME]))
    transmitter_pin = yield cg.gpio_pin_expression(config[CONF_TRANSMITTER_PIN])
    cg.add(var.set_transmitter_pin(transmitter_pin))
    cg.add(var.set_number_of_transmissions(config[CONF_NUMBER_OF_TRANSMISSIONS]))

