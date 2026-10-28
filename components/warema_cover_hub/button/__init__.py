import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import button
from esphome.const import CONF_DURATION, CONF_INTERNAL, ENTITY_CATEGORY_CONFIG

from .. import CONF_TRANSMITTER_ID, CONF_WAREMA_COVER_HUB_ID, warema_cover_hub_ns
from ..cover import WaremaCover

CONF_COVER_ID = "cover_id"
CONF_LEARN = "learn"

LearnButton = warema_cover_hub_ns.class_("LearnButton", button.Button, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_COVER_ID): cv.use_id(WaremaCover),
        cv.Required(CONF_LEARN): button.button_schema(
            LearnButton, entity_category=ENTITY_CATEGORY_CONFIG, icon="mdi:remote"
        )
        .extend(
            {
                # Teaching is done on site, so only the ESP's web server shows the button
                cv.Optional(CONF_INTERNAL, default=True): cv.boolean,
                # L on the remote has to be held about 3s; at most 255 repetitions, about 22s
                cv.Optional(CONF_DURATION, default="5s"): cv.All(
                    cv.positive_time_period_milliseconds,
                    cv.Range(
                        min=cv.TimePeriod(seconds=1), max=cv.TimePeriod(seconds=20)
                    ),
                ),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    }
)


def _final_validate(config):
    full_config = fv.full_config.get()
    cover_path = full_config.get_path_for_id(config[CONF_COVER_ID])[:-1]
    cover_config = full_config.get_config_for_path(cover_path)
    hub_path = full_config.get_path_for_id(cover_config[CONF_WAREMA_COVER_HUB_ID])[
        :-1
    ]
    hub_config = full_config.get_config_for_path(hub_path)
    if CONF_TRANSMITTER_ID not in hub_config:
        raise cv.Invalid(f"This button requires '{CONF_TRANSMITTER_ID}' on the hub")


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    cover = await cg.get_variable(config[CONF_COVER_ID])

    learn_config = config[CONF_LEARN]
    var = await button.new_button(learn_config)
    await cg.register_component(var, learn_config)
    cg.add(var.set_cover(cover))
    cg.add(var.set_duration(learn_config[CONF_DURATION]))
