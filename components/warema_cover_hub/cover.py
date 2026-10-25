import re

import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import cover
from esphome.const import CONF_CLOSE_DURATION, CONF_ID, CONF_OPEN_DURATION

from . import CONF_WAREMA_COVER_HUB_ID, WaremaCoverHub, warema_cover_hub_ns

CONF_DEVICECODE1 = "devicecode1"
CONF_DEVICECODE2 = "devicecode2"
CONF_COMMAND = "command"
CONF_REVERSE_DELAY = "reverse_delay"
CONF_FOLLOW_REMOTE = "follow_remote"
CONF_MEMBERS = "members"

WaremaCover = warema_cover_hub_ns.class_(
    "WaremaCover",
    cover.Cover,
    cg.Component,
    cg.Parented.template(WaremaCoverHub),
)


# The position is computed by dividing by these, so they must not be 0
DURATION_SCHEMA = cv.All(
    cv.positive_time_period_milliseconds,
    cv.Range(min=cv.TimePeriod(milliseconds=1)),
)


def validate_binary_string(length):
    def validator(value):
        value = cv.string(value)
        if not re.fullmatch(rf"[01]{{{length}}}", value):
            raise cv.Invalid(
                f"Value must be exactly {length} characters long and contain only 0 or 1."
            )
        return value

    return validator


CONFIG_SCHEMA = (
    cover.cover_schema(WaremaCover)
    .extend(
        {
            cv.GenerateID(CONF_WAREMA_COVER_HUB_ID): cv.use_id(WaremaCoverHub),
            cv.Required(CONF_OPEN_DURATION): DURATION_SCHEMA,
            cv.Required(CONF_CLOSE_DURATION): DURATION_SCHEMA,
            cv.Required(CONF_DEVICECODE1): validate_binary_string(9),
            cv.Required(CONF_DEVICECODE2): validate_binary_string(9),
            cv.Required(CONF_COMMAND): validate_binary_string(3),
            cv.Optional(
                CONF_REVERSE_DELAY, default="1s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FOLLOW_REMOTE, default=True): cv.boolean,
            cv.Optional(CONF_MEMBERS): cv.ensure_list(cv.use_id(WaremaCover)),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def _final_validate(config):
    if CONF_MEMBERS not in config:
        return
    full_config = fv.full_config.get()
    for member_id in config[CONF_MEMBERS]:
        if member_id == config[CONF_ID]:
            raise cv.Invalid("A cover cannot be its own member", path=[CONF_MEMBERS])
        member_path = full_config.get_path_for_id(member_id)[:-1]
        member_config = full_config.get_config_for_path(member_path)
        if CONF_MEMBERS in member_config:
            raise cv.Invalid(
                f"'{member_id}' is a group itself, groups cannot be nested",
                path=[CONF_MEMBERS],
            )


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_WAREMA_COVER_HUB_ID])

    hub = await cg.get_variable(config[CONF_WAREMA_COVER_HUB_ID])
    cg.add(hub.register_cover(var))

    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_devicecode1(config[CONF_DEVICECODE1]))
    cg.add(var.set_devicecode2(config[CONF_DEVICECODE2]))
    cg.add(var.set_command(config[CONF_COMMAND]))
    cg.add(var.set_reverse_delay(config[CONF_REVERSE_DELAY]))
    cg.add(var.set_follow_remote(config[CONF_FOLLOW_REMOTE]))
    for member_id in config.get(CONF_MEMBERS, []):
        member = await cg.get_variable(member_id)
        cg.add(var.add_member(member))
