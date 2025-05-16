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

#include "ba8206.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ba8206 {

void FanBA8206::setup()
{
  last_run_ = millis();

  auto restore = this->restore_state_();
  if (restore.has_value())
  {
    restore->apply(*this);
  }

  // Construct traits
  fan::FanTraits fan_traits(true, true, false, 3);
  fan_traits.set_supported_preset_modes({STR_FANMODE_NORMAL, STR_FANMODE_NATURE, STR_FANMODE_SLEEP});
  this->traits_ = fan_traits;

  this->fan_state_.speed = FAN8206_INVALID;
  this->fan_state_.oscillating = FAN8206_OSC_INVALID;
  this->fan_state_.timer = FAN8206_TIMER_INVALID;
  this->fan_state_.mode = FAN8206_MODE_INVALID;

  this->fan_timer_->publish_state(STR_FANTIMER_NONE_STR);
}

void FanBA8206::dump_config()
{
  LOG_FAN(TAG, "BA8206 Wall Fan", this);
}

void FanBA8206::update_state()
{
  uint32_t now = millis();
  if (now - this->last_run_ >= this->interval_ms_)
  {
    this->last_run_ = now;
    
    // interval task
    uint8_t data[2];
    bool success = this->read_bytes_raw(data, 2);
    if (!success)
    {
      ESP_LOGW(TAG, "I2C read error!");
      return;
    }
  
    uint8_t led_status = data[0] & 0xFE; // bit0 is Swing input only
    int led_scan_row[4] = {    (led_status >> 7) & 0x01,
                               (led_status >> 6) & 0x01,
                               (led_status >> 5) & 0x01,
                               (led_status >> 4) & 0x01 };
    int led_scan_com[3] = {    (led_status >> 3) & 0x01,
                               (led_status >> 2) & 0x01,
                               (led_status >> 1) & 0x01 };

    for (uint8_t r = 0; r < 4; r++)
    {
      if (led_scan_row[r] == 1)
      {
        for (uint8_t c = 0; c < 3; c++)
        {
          if (led_scan_com[c] == 0)
            this->led_on_counts[r][c]++;
        }
      }
    }

    sample_step++;
    if (sample_step >= 32)
    {
      bool led_state[4][3] = {};
      sample_step = 0;
      for (uint8_t r = 0; r < 4; r++)
      {
        for (uint8_t c = 0; c < 3; c++)
        {
          led_state[r][c] = (this->led_on_counts[r][c] > 2);
          this->led_on_counts[r][c] = 0;
        }
      }

      FanSpeed speed = FAN8206_OFF;
      if (led_state[0][1]) speed = FAN8206_HIGH;
      if (led_state[1][1]) speed = FAN8206_MEDIUM;
      if (led_state[2][1]) speed = FAN8206_LOW;

      FanOscilatting osc = FAN8206_OSC_OFF;
      if (led_state[0][2]) osc = FAN8206_OSC_ON;

      FanMode mode = FAN8206_MODE_OFF;
      if (led_state[3][2]) mode = FAN8206_MODE_NORMAL;
      if (led_state[3][1]) mode = FAN8206_MODE_NATURE;
      if (led_state[2][2]) mode = FAN8206_MODE_SLEEP;

      uint8_t timer = FAN8206_TIMER_OFF;
      if (led_state[3][0]) timer |= (FAN8206_TIMER_0_5H);
      if (led_state[2][0]) timer |= (FAN8206_TIMER_1H);
      if (led_state[1][0]) timer |= (FAN8206_TIMER_2H);
      if (led_state[0][0]) timer |= (FAN8206_TIMER_4H);

      bool state_change = false;

      // fan speed
      if (this->fan_state_.speed != speed)
      {
        this->fan_state_.speed = speed;
        if (this->fan_state_.speed == FAN8206_OFF)
        {
          this->state = false;
        }
        else
        {
          this->state = true;
          this->speed = static_cast<int>(this->fan_state_.speed);
        }
        state_change = true;
      }

      // fan oscillating
      if (this->fan_state_.oscillating != osc)
      {
        this->fan_state_.oscillating = osc;
        this->oscillating = static_cast<bool>(this->fan_state_.oscillating);
        state_change = true;
      }

      // fan mode
      if (this->fan_state_.mode != mode)
      {
        this->fan_state_.mode = mode;
        this->preset_mode = FANMODE_STR[static_cast<uint8_t>(this->fan_state_.mode)];
        state_change = true;
      }

      // fan timer
      if (this->fan_state_.timer != timer)
      {
        this->fan_state_.timer = timer;
        this->fan_timer_->set_fan_timer(this->fan_state_.timer);
      }

      if (state_change)
      {
        this->publish_state();
      }

    }

  }
}

void FanBA8206::loop()
{
  // skip if it is processing a command
  if (this->processing)
  {
    return;
  }

  this->update_state();
}

void FanBA8206::write_gpio(uint16_t value)
{
  uint8_t data[2];
  data[0] = ~value;
  data[1] = ~value >> 8;
  if (this->write(data, 2) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C write error!");
  }
}

void FanBA8206::process_command()
{
  if (this->button_queue.size() == 0)
  {
    this->update_state();
    this->processing = false;
    return;
  }

  uint8_t button_pressed = this->button_queue.front();
  this->write_gpio( 1 << button_pressed );
  this->set_timeout("button_off", 
                    50,
                    [this]() {
                      this->write_gpio(0);
                      this->button_queue.pop_front();
                      if (this->button_queue.size() > 0)
                      {
                        this->set_timeout(
                          "button_on",
                          50,
                          [this]() {
                            this->process_command();
                          });
                      }
                      else
                      {
                        this->process_command();
                      }
                    });
}

void FanBA8206::control(const fan::FanCall &call)
{
  // skip if it is processing a command
  if (this->processing)
  {
    return;
  }

  if (call.get_state().has_value())
  {
    bool newstate = *call.get_state();
    if (!newstate) // off
    {
      this->button_queue.push_back(FAN8206_BUTTON_OFF);
    }
    else // on
    {
      // if the fan is currently off, we need an additional ON pressed to on.
      if (!this->state)
      {
        if (this->independent_onoff_)
        {
          this->button_queue.push_back(FAN8206_BUTTON_ONOFF);
        }
        else
        {
          this->button_queue.push_back(FAN8206_BUTTON_ONSPEED);
        }
      }

      if (call.get_speed().has_value())
      {
        uint8_t currspeed = this->speed;
        uint8_t newspeed = *call.get_speed();
  
        newspeed = newspeed < currspeed ? newspeed + 3 : newspeed;
        this->button_queue.insert(this->button_queue.end(), newspeed-currspeed, FAN8206_BUTTON_SPEED);
      }

    }
  }

  if (call.get_oscillating().has_value())
  {
    if (this->oscillating != *call.get_oscillating())
    {
      this->button_queue.push_back(FAN8206_BUTTON_SWING);
    }
  }

  if (this->state && (call.get_preset_mode() != "") && (call.get_preset_mode() != STR_FANMODE_OFF))
  {
    if (call.get_preset_mode() != this->preset_mode)
    {
      uint8_t curr_preset_id = FANMODE_ID[this->preset_mode];
      uint8_t new_preset_id = FANMODE_ID[call.get_preset_mode()];
      new_preset_id = new_preset_id < curr_preset_id ? new_preset_id + 3 : new_preset_id;
      this->button_queue.insert(this->button_queue.end(), new_preset_id-curr_preset_id, FAN8206_BUTTON_MODE);
    }
  }

  if (this->button_queue.size() > 0)
  {
    // start processing
    this->processing = true;
    this->process_command();
  }


}

void FanBA8206Timer::dump_config()
{
    ESP_LOGCONFIG(TAG, "FanBA8206Timer:");
    LOG_TEXT_SENSOR("  Fan Timer: ", "fan_timer", this);
}

void FanBA8206Timer::setup()
{
}

void FanBA8206Timer::set_fan_timer(uint8_t timer)
{
  this->publish_state(STR_FANTIMER_STR[timer]);
}

void FanBA8206SetTimer::dump_config()
{
    ESP_LOGCONFIG(TAG, "FanBA8206SetTimer:");
    LOG_BUTTON("  Fan Set Timer: ", "fan_settimer", this);
}

void FanBA8206SetTimer::setup()
{
}

void FanBA8206SetTimer::press_action()
{
  if (this->fan_->state)
  {
    this->fan_->button_queue.push_back(FAN8206_BUTTON_TIMER);
    // start processing
    this->fan_->processing = true;
    this->fan_->process_command();
  }
}

}  // namespace ba8206
}  // namespace esphome
