/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
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

#include "displayman.h"

#include "../compat.h"

DisplayMan::DisplayMan() :
    _lcd(D3, D4, D5, D6, D7, D9),
    _lcd_prog(_lcd)
{
    _cycle_count = 0;
    _view_mode = DISPLAY_VIEW_SENSOR;
    _network_sensor_id = UINT8_MAX;
    _cloud_registered = false;
}

int DisplayMan::init(const std::string &version)
{
    _version_string = version;
    led_setup();
    led_set_color(IND_POWER, IND_COLOR_ON);
    _lcd.setBacklight(TextLCD_I2C::LightOn);
    _lcd.setCursor(TextLCD_I2C::CurOff_BlkOff);
#if MBED_CONF_APP_SELF_TEST
    self_test();
    led_setup();
#endif
    _view_mode = DISPLAY_VIEW_SENSOR;
    return 0;
}

void DisplayMan::set_downloading()
{
    _view_mode = DISPLAY_VIEW_DOWNLOAD;
    led_set_color(IND_FWUP, IND_COLOR_IN_PROGRESS, IND_FLAG_BLINK);
}

void DisplayMan::set_download_complete()
{
    led_set_color(IND_FWUP, IND_COLOR_SUCCESS);
}

void DisplayMan::set_progress(const std::string &message, uint32_t progress,
                              uint32_t total)
{
    _lcd_prog.set_progress(message, progress, total);
}

void DisplayMan::set_cloud_registered()
{
    led_set_color(IND_CLOUD, IND_COLOR_SUCCESS);
    _cloud_registered = true;
}

void DisplayMan::set_cloud_unregistered()
{
    led_set_color(IND_CLOUD, IND_COLOR_OFF);
    _cloud_registered = false;
}

void DisplayMan::set_installing()
{
    _view_mode = DISPLAY_VIEW_INSTALL;
    _lcd.printline(0, "Installing...    ");
    _lcd.printline(1, "");
    led_set_color(IND_FWUP, IND_COLOR_SUCCESS);
}

void DisplayMan::set_cloud_error()
{
    led_set_color(IND_CLOUD, IND_COLOR_FAILED);
    _cloud_registered = false;
}

void DisplayMan::init_network(const char *type)
{
    if (UINT8_MAX == _network_sensor_id) {
        _network_sensor_id = register_sensor(type);
    } else {
        set_sensor_name(_network_sensor_id, type);
    }
}

void DisplayMan::set_network_status(const std::string status)
{
    set_sensor_status(_network_sensor_id, status);
}

void DisplayMan::set_network_connecting()
{
    led_set_color(IND_WIFI, IND_COLOR_IN_PROGRESS, IND_FLAG_BLINK);
}

void DisplayMan::set_network_scanning()
{
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS, IND_FLAG_BLINK);
}

void DisplayMan::set_network_fail()
{
    led_set_color(IND_WIFI, IND_COLOR_FAILED);
}

void DisplayMan::set_network_success()
{
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS);
}

void DisplayMan::set_cloud_in_progress()
{
    led_set_color(IND_CLOUD, IND_COLOR_IN_PROGRESS, IND_FLAG_BLINK);
    _cloud_registered = false;
}

uint8_t DisplayMan::register_sensor(const std::string &name, enum INDICATOR_TYPES indicator)
{
    struct SensorDisplay s;

    s.name = name;
    s.status = "";
    s.indicator = indicator;

    if (indicator < IND_NO_TYPES) {
        led_set_color(indicator, IND_COLOR_SUCCESS_DIM);
    }

    _sensors.push_back(s);

    return _sensors.size() - 1;
}

struct DisplayMan::SensorDisplay *
DisplayMan::find_sensor(const std::string &name)
{
    struct SensorDisplay *s;
    std::vector<struct SensorDisplay>::iterator it;

    for (it = _sensors.begin() ; it != _sensors.end(); ++it) {
        s = &(*it);
        if (s->name == name) {
            return s;
        }
    }

    return NULL;
}

void DisplayMan::set_sensor_status(struct SensorDisplay *s,
                                   const std::string status)
{
    s->status = status;
    if (_cloud_registered && s->indicator < IND_NO_TYPES) {
        led_set_color(s->indicator, IND_COLOR_SENSOR_DATA, IND_FLAG_ONCE);
        led_set_color(IND_CLOUD,
                      IND_COLOR_SENSOR_DATA,
                      IND_FLAG_ONCE | IND_FLAG_LATER);
    }
}

void DisplayMan::set_sensor_status(uint8_t sensor_id, const std::string status)
{
    struct SensorDisplay *s;

    if (sensor_id >= _sensors.size()) {
        return;
    }

    s = &_sensors[sensor_id];
    set_sensor_status(s, status);
}

void DisplayMan::set_sensor_status(const std::string name,
                                   const std::string status)
{
    struct SensorDisplay *s;

    s = find_sensor(name);
    if (NULL == s) {
        return;
    }

    set_sensor_status(s, status);
}

void DisplayMan::set_sensor_name(uint8_t sensor_id, const std::string name)
{
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].name = name;
    }
}

void DisplayMan::cycle_status()
{
    char line[17];

    /* top line */
    _lcd.printlinef(0, "Version: %s", _version_string.c_str());

    /* bottom line */
    if (_sensors.size() > 0) {
        snprintf(line, sizeof(line), "%s: %s",
                 _sensors[_active_sensor].name.c_str(),
                 _sensors[_active_sensor].status.c_str());
        _lcd.printline(1, line);
        _active_sensor = (_active_sensor + 1) % _sensors.size();
    }
}

void DisplayMan::refresh()
{
    led_post();

    if (_view_mode == DISPLAY_VIEW_SENSOR) {
        if (_cycle_count >= (DISPLAY_PAGING_UPDATE_PERIOD_MS - 1)/DISPLAY_UPDATE_PERIOD_MS) {
            _cycle_count = 0;
            cycle_status();
        }
    } else if (_view_mode == DISPLAY_VIEW_DOWNLOAD) {
        _lcd_prog.refresh();
    } else if (_view_mode == DISPLAY_VIEW_INSTALL) {
        /* nothing for now */
    } else if (_view_mode == DISPLAY_VIEW_SELF_TEST) {
        _lcd_prog.refresh();
    }

    _cycle_count++;
}

void DisplayMan::self_test()
{
    uint32_t i;
    INDICATOR_TYPES indicators[] = {IND_POWER, IND_WIFI,  IND_CLOUD,
                                    IND_FWUP,  IND_LIGHT, IND_TEMP};

    for (i = 0; i < ARRAY_SIZE(indicators); i++) {
        led_set_color(indicators[i], IND_COLOR_IN_PROGRESS);
    }
    for (i = 0; i <= 90; i++) {
        _view_mode = DISPLAY_VIEW_SELF_TEST;
        set_progress("Self test", i, 90);
        refresh();
        Thread::wait(10);
    }
}
