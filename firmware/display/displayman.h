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

// ****************************************************************************
//  Workplace Environmental Monitor
//
//  This is a reference deployment application utilizing mbed cloud 1.2.
//
//  By the ARM Reference Design Team
// ****************************************************************************

// Abstraction of hardware display

#ifndef __DISPLAYMAN_H__
#define __DISPLAYMAN_H__

#include "lcdprogress.h"
#include "ledman.h"
#include "multiaddrlcd.h"

#include <string>
#include <vector>

#define DISPLAY_UPDATE_PERIOD_MS 180
#define DISPLAY_PAGING_UPDATE_PERIOD_MS 1980 // this should be a multiple of the LED update period

class DisplayMan;

void thread_display_update(DisplayMan *display);

class DisplayMan
{
public:
    DisplayMan();
    int init(const std::string &version);
    void set_cloud_registered();
    void set_cloud_unregistered();
    void set_cloud_in_progress();
    void set_cloud_error();
    void set_downloading();
    void set_download_complete();
    void set_installing();
    void set_progress(const std::string &message, uint32_t progress,
                      uint32_t total);
    void set_network_status(const std::string status);
    void set_network_connecting();
    void set_network_scanning();
    void set_network_fail();
    void set_network_success();
    void init_network(const char *type);
    /*returns sesor id*/
    uint8_t register_sensor(const std::string &name, enum INDICATOR_TYPES indicator = IND_NO_TYPES);
    void set_sensor_status(uint8_t sensor_id, const std::string status);
    void set_sensor_status(const std::string name, const std::string status);
    void set_sensor_name(uint8_t sensor_id, const std::string name);
    void cycle_status();
    void refresh();
    void self_test();

private:
    struct SensorDisplay {
        std::string name;
        std::string status;
        INDICATOR_TYPES indicator;
    };

    enum ViewMode {
        DISPLAY_VIEW_SENSOR,
        DISPLAY_VIEW_DOWNLOAD,
        DISPLAY_VIEW_INSTALL,
        DISPLAY_VIEW_SELF_TEST
    };
    MultiAddrLCD _lcd;
    LCDProgress _lcd_prog;

    std::vector<SensorDisplay> _sensors;
    uint8_t _network_sensor_id;
    uint8_t _active_sensor;
    enum ViewMode _view_mode;
    std::string _version_string;
    bool _cloud_registered;

    uint64_t _cycle_count;

    struct SensorDisplay *find_sensor(const std::string &name);
    void set_sensor_status(struct SensorDisplay *s, const std::string);
};

#endif
