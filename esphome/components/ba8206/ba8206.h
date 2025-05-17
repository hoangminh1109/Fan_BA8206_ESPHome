/*
* Copyright 2025 Hoang Minh
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include <deque>
#include <map>

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace ba8206 {

static const char *const TAG = "ba8206.fan";

enum FanSpeed {
  FAN8206_OFF = 0,
  FAN8206_LOW = 1,
  FAN8206_MEDIUM = 2,
  FAN8206_HIGH = 3,
  FAN8206_INVALID = 100
};

enum FanOscilatting {
  FAN8206_OSC_OFF = 0,
  FAN8206_OSC_ON = 1,
  FAN8206_OSC_INVALID = 100,
};

#define FAN8206_TIMER_OFF     0x00
#define FAN8206_TIMER_0_5H    0x01
#define FAN8206_TIMER_1H      0x02
#define FAN8206_TIMER_2H      0x04
#define FAN8206_TIMER_4H      0x08
#define FAN8206_TIMER_INVALID 100

enum FanMode {
  FAN8206_MODE_OFF = 0,
  FAN8206_MODE_NORMAL = 1,
  FAN8206_MODE_NATURE = 2,
  FAN8206_MODE_SLEEP = 3,
  FAN8206_MODE_INVALID = 100,
};

struct FanState {
  FanSpeed speed;
  FanOscilatting oscillating;
  FanMode mode;
  uint8_t timer;
};

#define STR_FANMODE_OFF     "---"
#define STR_FANMODE_NORMAL  "🌬️ Normal"
#define STR_FANMODE_NATURE  "🍃 Nature"
#define STR_FANMODE_SLEEP   "💤 Sleep"

static std::map<uint8_t, std::string> FANMODE_STR = {
  {FAN8206_MODE_OFF, STR_FANMODE_OFF},
  {FAN8206_MODE_NORMAL, STR_FANMODE_NORMAL},
  {FAN8206_MODE_NATURE, STR_FANMODE_NATURE},
  {FAN8206_MODE_SLEEP, STR_FANMODE_SLEEP}
};

static std::map<std::string, uint8_t> FANMODE_ID = {
  {STR_FANMODE_OFF, FAN8206_MODE_OFF},
  {STR_FANMODE_NORMAL, FAN8206_MODE_NORMAL},
  {STR_FANMODE_NATURE, FAN8206_MODE_NATURE},
  {STR_FANMODE_SLEEP, FAN8206_MODE_SLEEP}
};

#define STR_FANTIMER_NONE_STR    "---"
#define STR_FANTIMER_0_5H_STR    "0.5h"
#define STR_FANTIMER_1_0H_STR    "1.0h"
#define STR_FANTIMER_1_5H_STR    "1.5h"
#define STR_FANTIMER_2_0H_STR    "2.0h"
#define STR_FANTIMER_2_5H_STR    "2.5h"
#define STR_FANTIMER_3_0H_STR    "3.0h"
#define STR_FANTIMER_3_5H_STR    "3.5h"
#define STR_FANTIMER_4_0H_STR    "4.0h"
#define STR_FANTIMER_4_5H_STR    "4.5h"
#define STR_FANTIMER_5_0H_STR    "5.0h"
#define STR_FANTIMER_5_5H_STR    "5.5h"
#define STR_FANTIMER_6_0H_STR    "6.0h"
#define STR_FANTIMER_6_5H_STR    "6.5h"
#define STR_FANTIMER_7_0H_STR    "7.0h"
#define STR_FANTIMER_7_5H_STR    "7.5h"

// Array using the defines
static const char *STR_FANTIMER_STR[] = {
    STR_FANTIMER_NONE_STR,
    STR_FANTIMER_0_5H_STR,
    STR_FANTIMER_1_0H_STR,
    STR_FANTIMER_1_5H_STR,
    STR_FANTIMER_2_0H_STR,
    STR_FANTIMER_2_5H_STR,
    STR_FANTIMER_3_0H_STR,
    STR_FANTIMER_3_5H_STR,
    STR_FANTIMER_4_0H_STR,
    STR_FANTIMER_4_5H_STR,
    STR_FANTIMER_5_0H_STR,
    STR_FANTIMER_5_5H_STR,
    STR_FANTIMER_6_0H_STR,
    STR_FANTIMER_6_5H_STR,
    STR_FANTIMER_7_0H_STR,
    STR_FANTIMER_7_5H_STR
};

#define FAN8206_BUTTON_OFF      7
#define FAN8206_BUTTON_ONOFF    7
#define FAN8206_BUTTON_TIMER    6
#define FAN8206_BUTTON_SPEED    5
#define FAN8206_BUTTON_ONSPEED  5
#define FAN8206_BUTTON_MODE     4
#define FAN8206_BUTTON_SWING    0

class FanBA8206;

class FanBA8206Timer : public text_sensor::TextSensor, public Component
{
public:
    void setup() override;
    void dump_config() override;
    void set_parent_fan(FanBA8206 *fan) { this->fan_ = fan; }
    void set_fan_timer(uint8_t timer);

private:
    FanBA8206 *fan_{nullptr};
};

class FanBA8206SetTimer : public button::Button, public Component
{
public:
    void setup() override;
    void dump_config() override;
    void press_action() override;
    void set_parent_fan(FanBA8206 *fan) { this->fan_ = fan; }

private:
    FanBA8206 *fan_{nullptr};
};


class FanBA8206 : public Component,
                public fan::Fan,
                public i2c::I2CDevice
{
public:
  FanBA8206() {}
  void setup() override;
  void dump_config() override;
  void loop() override;
  void set_interval_ms(int interval_ms) { this->interval_ms_ = interval_ms; }
  void set_independent_onoff(bool independent_onoff) {this->independent_onoff_ = independent_onoff;}
  void set_fan_timer(FanBA8206Timer *fan_timer) { this->fan_timer_ = fan_timer; }
  void set_fan_settimer(FanBA8206SetTimer *fan_settimer) { this->fan_settimer_ = fan_settimer; }
  fan::FanTraits get_traits() override { return this->traits_; }
  std::deque<uint8_t> button_queue{};
  void process_command();
  bool processing{false};

protected:
  void control(const fan::FanCall &call) override;

private:
  fan::FanTraits traits_;
  uint32_t last_run_{0};
  uint8_t sample_step{0};
  uint8_t led_on_counts[4][3] = {};
  FanState fan_state_;

  int interval_ms_{0};
  bool independent_onoff_{false};

  FanBA8206Timer *fan_timer_{nullptr};
  FanBA8206SetTimer *fan_settimer_{nullptr};

  void write_gpio(uint16_t value);
  void update_state();
};

}  // namespace ba8206
}  // namespace esphome
