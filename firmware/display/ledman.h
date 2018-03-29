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

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
enum INDICATOR_TYPES {
    IND_POWER = 0,
    IND_WIFI,
    IND_CLOUD,
    IND_FWUP,
    IND_LIGHT,
    IND_SOUND,
    IND_TEMP,
    IND_HUMIDITY = IND_TEMP,
    IND_NO_TYPES
};

// LEDs are BGR instead of RGB
#define BLACK 0x000000
#define COMMI_RED 0x0000FF
#define CANADIAN_BLUE 0xFF0000
#define SLIMER_GREEN 0x006400
#define SNOW_YELLOW 0x00FFFF
#define MIDNIGHT_GREEN 0x003200
#define MIDNIGHT_BLUE 0x0F0000
#define SUMMER_BLUE 0xFEED00

#define COMMI_RED_HI 0x0000FF
#define COMMI_RED_LO 0x000000
#define CANADIAN_BLUE_HI 0x000000
#define CANADIAN_BLUE_LO 0x00FFFF
#define SLIMER_GREEN_HI 0x000000
#define SLIMER_GREEN_LO 0x0000FF
#define SNOW_YELLOW_HI 0xFFFFFF
#define SNOW_YELLOW_LO 0x000000
#define MIDNIGHT_GREEN_HI 0x646400
#define MIDNIGHT_GREEN_LO 0x000000
#define MIDNIGHT_BLUE_HI 0x000000
#define MIDNIGHT_BLUE_LO 0x000F0F
#define SUMMER_BLUE_HI 0x000000
#define SUMMER_BLUE_LO 0xFE00ED

#ifndef MBED_CONF_APP_COLOR_IN_PROGRESS
#define MBED_CONF_APP_COLOR_IN_PROGRESS SNOW_YELLOW
#endif
#ifndef MBED_CONF_APP_COLOR_FAILED
#define MBED_CONF_APP_COLOR_FAILED COMMI_RED
#endif
#ifndef MBED_CONF_APP_COLOR_ON
#define MBED_CONF_APP_COLOR_ON SLIMER_GREEN
#endif
#ifndef MBED_CONF_APP_COLOR_SUCCESS
#define MBED_CONF_APP_COLOR_SUCCESS CANADIAN_BLUE
#endif
#ifndef MBED_CONF_APP_COLOR_SUCCESS_DIM
#define MBED_CONF_APP_COLOR_SUCCESS_DIM MIDNIGHT_BLUE
#endif
#ifndef MBED_CONF_APP_COLOR_SENSOR_DATA
#define MBED_CONF_APP_COLOR_SENSOR_DATA SUMMER_BLUE
#endif

enum INDICATOR_COLORS {
    IND_COLOR_OFF = BLACK,
    IND_COLOR_IN_PROGRESS = MBED_CONF_APP_COLOR_IN_PROGRESS,
    IND_COLOR_FAILED = MBED_CONF_APP_COLOR_FAILED,
    IND_COLOR_ON = MBED_CONF_APP_COLOR_ON,
    IND_COLOR_SUCCESS = MBED_CONF_APP_COLOR_SUCCESS,
    IND_COLOR_SUCCESS_DIM = MBED_CONF_APP_COLOR_SUCCESS_DIM,
    IND_COLOR_SENSOR_DATA = MBED_CONF_APP_COLOR_SENSOR_DATA
};

enum INDICATOR_FLAGS {
    IND_FLAG_NONE = 0,
    IND_FLAG_BLINK = 1,   // blink the LED indefinitely
    IND_FLAG_ONCE = 2,    // show the color once and then revert to the previous color
    IND_FLAG_LATER = 4,   // show the color next cycle
};

void led_flags_set(int led_name, int flags);  // overwrite all flags
int led_flags_get(int led_name);              // return all flags
bool led_flag_is_set(int led_name, int flag); // check whether a particular flag is set
void led_clear_flag(int led_name, int flag);  // clear one flag
void led_set_color(enum INDICATOR_TYPES led_name, int led_color, int flags = IND_FLAG_NONE);
void led_post(void);                          // update hardware LEDs
void led_setup(void);
