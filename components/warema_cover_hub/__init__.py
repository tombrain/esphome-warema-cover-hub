import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome import automation
from esphome.components import remote_base
from esphome.const import CONF_ID, CONF_IDLE, CONF_TRIGGER_ID

CODEOWNERS = ["@tombrain"]
DEPENDENCIES = ["remote_base"]
AUTO_LOAD = ["cover"]
MULTI_CONF = True

CONF_WAREMA_COVER_HUB_ID = "warema_cover_hub_id"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"
CONF_NUMBER_OF_TRANSMISSIONS = "number_of_transmissions"
CONF_ON_UNASSIGNED = "on_unassigned"

# remote_receiver ends a capture after `idle` without an edge, at either level. A sync is
# up to 6780 µs high (5000 µs plus a merged half bit on each side), so a shorter idle cuts
# the telegram at its syncs.
MIN_RECEIVER_IDLE_US = 7000

warema_cover_hub_ns = cg.esphome_ns.namespace("warema_cover_hub")
WaremaCoverHub = warema_cover_hub_ns.class_(
    "WaremaCoverHub", cg.Component, remote_base.RemoteReceiverListener
)
UnassignedTrigger = warema_cover_hub_ns.class_(
    "UnassignedTrigger",
    automation.Trigger.template(
        cg.std_string, cg.std_string, cg.std_string, cg.std_string, cg.std_string
    ),
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
            cv.Optional(CONF_ON_UNASSIGNED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UnassignedTrigger)}
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_TRANSMITTER_ID, CONF_RECEIVER_ID),
)


def _final_validate(config):
    if CONF_RECEIVER_ID not in config:
        if CONF_ON_UNASSIGNED in config:
            raise cv.Invalid(
                f"'{CONF_ON_UNASSIGNED}' requires '{CONF_RECEIVER_ID}'",
                path=[CONF_ON_UNASSIGNED],
            )
        return
    full_config = fv.full_config.get()
    receiver_path = full_config.get_path_for_id(config[CONF_RECEIVER_ID])[:-1]
    receiver_config = full_config.get_config_for_path(receiver_path)
    idle = receiver_config.get(CONF_IDLE)
    if idle is not None and idle.total_microseconds < MIN_RECEIVER_IDLE_US:
        raise cv.Invalid(
            f"The receiver's '{CONF_IDLE}' must be at least "
            f"{MIN_RECEIVER_IDLE_US // 1000}ms, Warema syncs are up to 6.8ms long",
            path=[CONF_RECEIVER_ID],
        )


FINAL_VALIDATE_SCHEMA = _final_validate


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

    for conf in config.get(CONF_ON_UNASSIGNED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger,
            [
                (cg.std_string, "key"),
                (cg.std_string, "command"),
                (cg.std_string, "devicecode1"),
                (cg.std_string, "devicecode2"),
                (cg.std_string, "description"),
            ],
            conf,
        )
