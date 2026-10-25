import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import event
from esphome.const import DEVICE_CLASS_BUTTON

from .. import (
    CONF_RECEIVER_ID,
    CONF_WAREMA_COVER_HUB_ID,
    WaremaCoverHub,
    warema_cover_hub_ns,
)
from ..cover import (
    CONF_COMMAND,
    CONF_DEVICECODE1,
    CONF_DEVICECODE2,
    WaremaCover,
    validate_binary_string,
)

CONF_COVER_ID = "cover_id"

EVENT_TYPES = ["open", "close", "stop", "function_a", "function_b", "function_c"]

RemoteEvent = warema_cover_hub_ns.class_(
    "RemoteEvent",
    event.Event,
    cg.Component,
    cg.Parented.template(WaremaCoverHub),
)

CONFIG_SCHEMA = cv.All(
    event.event_schema(RemoteEvent, device_class=DEVICE_CLASS_BUTTON, icon="mdi:remote")
    .extend(
        {
            cv.GenerateID(CONF_WAREMA_COVER_HUB_ID): cv.use_id(WaremaCoverHub),
            cv.Optional(CONF_COVER_ID): cv.use_id(WaremaCover),
            cv.Inclusive(CONF_COMMAND, "codes"): validate_binary_string(3),
            cv.Inclusive(CONF_DEVICECODE1, "codes"): validate_binary_string(9),
            cv.Inclusive(CONF_DEVICECODE2, "codes"): validate_binary_string(9),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_exactly_one_key(CONF_COVER_ID, CONF_COMMAND),
)


def _final_validate(config):
    full_config = fv.full_config.get()
    hub_path = full_config.get_path_for_id(config[CONF_WAREMA_COVER_HUB_ID])[:-1]
    hub_config = full_config.get_config_for_path(hub_path)
    if CONF_RECEIVER_ID not in hub_config:
        raise cv.Invalid(f"This event requires '{CONF_RECEIVER_ID}' on the hub")


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = await event.new_event(config, event_types=EVENT_TYPES)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_WAREMA_COVER_HUB_ID])

    if CONF_COVER_ID in config:
        cover = await cg.get_variable(config[CONF_COVER_ID])
        cg.add(var.set_cover(cover))
    else:
        cg.add(
            var.set_codes(
                config[CONF_COMMAND],
                config[CONF_DEVICECODE1],
                config[CONF_DEVICECODE2],
            )
        )
