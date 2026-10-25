import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_base
from esphome.const import CONF_ID

CODEOWNERS = ["@tombrain"]
DEPENDENCIES = ["remote_base"]
AUTO_LOAD = ["cover"]
MULTI_CONF = True

CONF_WAREMA_COVER_HUB_ID = "warema_cover_hub_id"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"
CONF_NUMBER_OF_TRANSMISSIONS = "number_of_transmissions"

warema_cover_hub_ns = cg.esphome_ns.namespace("warema_cover_hub")
WaremaCoverHub = warema_cover_hub_ns.class_(
    "WaremaCoverHub", cg.Component, remote_base.RemoteReceiverListener
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WaremaCoverHub),
            cv.Optional(CONF_TRANSMITTER_ID): cv.use_id(
                remote_base.RemoteTransmitterBase
            ),
            cv.Optional(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
            cv.Optional(CONF_NUMBER_OF_TRANSMISSIONS, default=3): cv.int_range(
                min=1, max=10
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_TRANSMITTER_ID, CONF_RECEIVER_ID),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_TRANSMITTER_ID in config:
        transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
        cg.add(var.set_transmitter(transmitter))
    if CONF_RECEIVER_ID in config:
        receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
        remote_base.add_listener(receiver, var)
        cg.add(var.set_has_receiver(True))
    cg.add(var.set_number_of_transmissions(config[CONF_NUMBER_OF_TRANSMISSIONS]))
