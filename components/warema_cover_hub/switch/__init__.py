import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import switch
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import (
    CONF_RECEIVER_ID,
    CONF_WAREMA_COVER_HUB_ID,
    WaremaCoverHub,
    warema_cover_hub_ns,
)

ShowCodesSwitch = warema_cover_hub_ns.class_(
    "ShowCodesSwitch",
    switch.Switch,
    cg.Component,
    cg.Parented.template(WaremaCoverHub),
)

CONFIG_SCHEMA = (
    switch.switch_schema(
        ShowCodesSwitch,
        default_restore_mode="RESTORE_DEFAULT_ON",
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:eye",
    )
    .extend({cv.GenerateID(CONF_WAREMA_COVER_HUB_ID): cv.use_id(WaremaCoverHub)})
    .extend(cv.COMPONENT_SCHEMA)
)


def _final_validate(config):
    full_config = fv.full_config.get()
    hub_path = full_config.get_path_for_id(config[CONF_WAREMA_COVER_HUB_ID])[:-1]
    hub_config = full_config.get_config_for_path(hub_path)
    if CONF_RECEIVER_ID not in hub_config:
        raise cv.Invalid(f"This switch requires '{CONF_RECEIVER_ID}' on the hub")


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_WAREMA_COVER_HUB_ID])
