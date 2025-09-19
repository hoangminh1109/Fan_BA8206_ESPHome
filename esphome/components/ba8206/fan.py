# Copyright 2025 Minh Hoang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, i2c, text_sensor, button
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_ID,
    CONF_NAME,
    CONF_DISABLED_BY_DEFAULT
)

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["fan", "remote_base", "i2c", "text_sensor", "button"]

ba8206_ns = cg.esphome_ns.namespace('ba8206')
FanBA8206 = ba8206_ns.class_(
    "FanBA8206",
    cg.Component,
    fan.Fan,
    i2c.I2CDevice,
    )
FanBA8206Timer = ba8206_ns.class_(
    'FanBA8206Timer',
    text_sensor.TextSensor,
    cg.Component
    )
FanBA8206SetTimer = ba8206_ns.class_(
    'FanBA8206SetTimer',
    button.Button,
    cg.Component
    )

CONF_FANTIMER_ID = "fantimer_id"
CONF_FANSETTIMER_ID = "fansettimer_id"
CONF_INTERVAL_MS = "interval"
CONF_SEPARATED_ONOFF = "independent_onoff"

CONFIG_SCHEMA = (fan.fan_schema(FanBA8206).extend({
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(FanBA8206),
            cv.GenerateID(CONF_FANTIMER_ID): cv.declare_id(FanBA8206Timer),
            cv.GenerateID(CONF_FANSETTIMER_ID): cv.declare_id(FanBA8206SetTimer),
            cv.Optional(CONF_INTERVAL_MS, default=1): cv.int_range(min=1),
            cv.Optional(CONF_SEPARATED_ONOFF, default=False): cv.boolean,
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x20))
    )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_independent_onoff(config[CONF_SEPARATED_ONOFF]))

    # Fan timer text_sensor
    fantimer_default_config = { CONF_ID: config[CONF_FANTIMER_ID],
                                CONF_NAME: "Timer",
                                CONF_DISABLED_BY_DEFAULT: False}
    fantimer = cg.new_Pvariable(config[CONF_FANTIMER_ID])
    await text_sensor.register_text_sensor(fantimer, fantimer_default_config)
    await cg.register_component(fantimer, fantimer_default_config)
    cg.add(fantimer.set_parent_fan(var))
    cg.add(var.set_fan_timer(fantimer))

    # Fan set timer button
    fansettimer_default_config = { CONF_ID: config[CONF_FANSETTIMER_ID],
                                CONF_NAME: "Set Timer",
                                CONF_DISABLED_BY_DEFAULT: False}
    fansettimer = cg.new_Pvariable(config[CONF_FANSETTIMER_ID])
    await button.register_button(fansettimer, fansettimer_default_config)
    await cg.register_component(fansettimer, fansettimer_default_config)
    cg.add(fansettimer.set_parent_fan(var))
    cg.add(var.set_fan_settimer(fansettimer))

    cg.add(var.set_interval_ms(config[CONF_INTERVAL_MS]))