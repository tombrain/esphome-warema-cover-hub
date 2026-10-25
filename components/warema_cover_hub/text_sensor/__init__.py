import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import text_sensor
from esphome.const import CONF_NAME

from .. import (
    CONF_RECEIVER_ID,
    CONF_WAREMA_COVER_HUB_ID,
    WaremaCoverHub,
    warema_cover_hub_ns,
)

ReceivedCodeTextSensor = warema_cover_hub_ns.class_(
    "ReceivedCodeTextSensor",
    text_sensor.TextSensor,
    cg.Component,
    cg.Parented.template(WaremaCoverHub),
)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(
        ReceivedCodeTextSensor,
        icon="mdi:remote",
    )
    .extend(
        {
            cv.GenerateID(CONF_WAREMA_COVER_HUB_ID): cv.use_id(WaremaCoverHub),
            cv.Optional(CONF_NAME, default="Received Warema code"): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def _final_validate(config):
    full_config = fv.full_config.get()
    hub_path = full_config.get_path_for_id(config[CONF_WAREMA_COVER_HUB_ID])[:-1]
    hub_config = full_config.get_config_for_path(hub_path)
    if CONF_RECEIVER_ID not in hub_config:
        raise cv.Invalid(f"This text sensor requires '{CONF_RECEIVER_ID}' on the hub")


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_WAREMA_COVER_HUB_ID])
