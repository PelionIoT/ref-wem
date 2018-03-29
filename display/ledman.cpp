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
#include "ledman.h"
#include "../compat.h"

#include "PCA9956A.h"
#include "PinNames.h"
#include <vector>

/* LEDController template logic.
 */
class BaseController {
public:
    BaseController()
    {
    }
    ~BaseController()
    {
    }

    /* Set flag for particular LED.
     *
     * @param led_name Which LED to set the flag on.
     * @param flags The flag to set.
     */
    void flags_set(int led_name, int flags)
    {
        LED_COLOR[led_name] &= 0x00FFFFFF;
        LED_COLOR[led_name] |= flags << 24;
        LED_COLOR_TEMP[led_name] &= 0x00FFFFFF;
        LED_COLOR_TEMP[led_name] |= flags << 24;
    }

    /* Get the current flags set on a LED.
     *
     * @param led_name Which LED to return flags from.
     * @return Returns the flags currently set on led_name.
     */
    int flags_get(int led_name)
    {
        return LED_COLOR[led_name] >> 24;
    }

    /* Check is a flag is currently set on a LED.
     *
     * @param led_name Which LED to check for the flag.
     * @param flag The particular flag to check for.
     * @return Returns true if the flag is set, false otherwise.
     */
    bool flag_is_set(int led_name, int flag)
    {
        return flags_get(led_name) & flag;
    }

    /* Clears a particular flag on a LED.
     *
     * @param led_name Which LED to clear the flag on.
     * @param flag The flag to clear.
     */
    void clear_flag(int led_name, int flag)
    {
        LED_COLOR[led_name] &= ~(flag << 24);
        LED_COLOR_TEMP[led_name] &= ~(flag << 24);
    }

    /* Set a particular color and flag on a LED.
     *
     * @param led_name Which LED to set the flag and change color.
     * @param led_color The color of the LED.
     * @param flags The flags to set on the LED.
     */
    void set_color(enum INDICATOR_TYPES led_name, int led_color, int flags)
    {
        /* colors are only 24-bits */
        int color = led_color & 0x00FFFFFF;

        if (led_name >= IND_NO_TYPES) {
            return;
        }

        if ((flags & IND_FLAG_ONCE) || (flags & IND_FLAG_LATER)) {
            LED_COLOR_TEMP[led_name] = color;
        } else {
            LED_COLOR[led_name] = color;
        }
        flags_set(led_name, flags);
    }

    void led_post(void)
    {
        int idx;

        for (idx = 0; idx < IND_NO_TYPES; ++idx) {
            int color = LED_COLOR[idx];
            if (flag_is_set(idx, IND_FLAG_BLINK) &&
                LED_HARDWARE[idx] != IND_COLOR_OFF) {
                color = IND_COLOR_OFF;
            }
            if (flag_is_set(idx, IND_FLAG_LATER)) {
                if (!flag_is_set(idx, IND_FLAG_ONCE)) {
                    LED_COLOR[idx] = LED_COLOR_TEMP[idx];
                }
                clear_flag(idx, IND_FLAG_LATER);
            } else if (flag_is_set(idx, IND_FLAG_ONCE)) {
                color = LED_COLOR_TEMP[idx];
                clear_flag(idx, IND_FLAG_ONCE);
            }
            LED_HARDWARE[idx] = color & 0x00FFFFFF;
        }
        led_update();
    }

    void led_setup(void)
    {
        for (int i = 0; i < IND_NO_TYPES; i++) {
            LED_HARDWARE[i] = IND_COLOR_OFF;
            LED_COLOR[i] = 0x0;
            LED_COLOR_TEMP[i] = 0x0;
        }
        led_init();
    }

protected:
    /* Virtual function for updating LEDs which derived classes should define.
     */
    virtual void led_update(void) = 0;
    /* Virtual function for setting up LEDs which derviced classes should
     * define. */
    virtual void led_init(void) = 0;

    int LED_HARDWARE[IND_NO_TYPES]; /* color currently being displayed */
    /* really poor design, but we store the indicator flags in the upper 8-bits
     * and the color in the lower 24-bits. Once blinking is disabled then we
     * just update the LED_HARDWARE field with the original color.
     */
    int LED_COLOR[IND_NO_TYPES];
    int LED_COLOR_TEMP[IND_NO_TYPES]; /* tempoary colors */
};


/** Struct definition for the PWM LEDs */
class RGBLED {
public:
    RGBLED(LedPwmOutCC r, LedPwmOutCC g, LedPwmOutCC b)
        : red(r), green(g), blue(b)
    {
    }
    LedPwmOutCC red;
    LedPwmOutCC green;
    LedPwmOutCC blue;
};

class LEDController : public BaseController {
public:
    LEDController() : led_ctrl(I2C_SDA, I2C_SCL, 0x02)
    {
        LedPinName pins[] = {
            L0,  L1,  L2,  // power
            L3,  L4,  L5,  // wifi
            L6,  L7,  L8,  // cloud
            L9,  L10, L11, // firmware upload
            L12, L13, L14, // light
            L15, L16, L17, // temperature and humidity
            L18, L19, L20  // sound
        };
        unsigned i;

        led_strip.reserve(IND_NO_TYPES);

        for (i = 0; i + 2 < sizeof(pins) / sizeof(pins[0]); i += 3) {
            LedPwmOutCC r(led_ctrl, pins[i]);
            LedPwmOutCC g(led_ctrl, pins[i + 1]);
            LedPwmOutCC b(led_ctrl, pins[i + 2]);
            RGBLED rgbled(r, g, b);
            led_strip.push_back(rgbled);
        }
    }

    ~LEDController()
    {
    }

private:
    /** Updates the hardware LEDs based on the internal colors and flags set.
     */
    void led_update(void)
    {
        int idx;

        for (idx = 0; idx < IND_NO_TYPES; ++idx) {
            /* set the hardware LEDs */
            led_strip[idx].red.pwm(getRed(idx));
            led_strip[idx].green.pwm(getGreen(idx));
            led_strip[idx].blue.pwm(getBlue(idx));
        }
    }

    /** Setup the internal state of the LED colors and flags.
     */
    void led_init(void)
    {
        for (int i = 0; i < IND_NO_TYPES; i++) {
            led_strip[i].red.current(1.0f);
            led_strip[i].green.current(1.0f);
            led_strip[i].blue.current(1.0f);
        }
    }

    PCA9956A led_ctrl;             /* physical PWM LED controller */
    std::vector<RGBLED> led_strip; /* set of RGB LEDs attached to controller */

    /* simple helper functions for converting 24-bit color to 8-bit, then floats
     */
    float getRed(int led_name)
    {
        return (float)(LED_HARDWARE[led_name] & 0x000000FF) / 256.0f;
    }
    float getGreen(int led_name)
    {
        return (float)((LED_HARDWARE[led_name] & 0x0000FF00) >> 8) / 256.0f;
    }
    float getBlue(int led_name)
    {
        return (float)((LED_HARDWARE[led_name] & 0x00FF0000) >> 16) / 256.0f;
    }
};

LEDController ledctrl;

// ****************************************************************************
// Functions
// ****************************************************************************
void led_flags_set(int led_name, int flags)
{
    ledctrl.flags_set(led_name, flags);
}

int led_flags_get(int led_name)
{
    return ledctrl.flags_get(led_name);
}

bool led_flag_is_set(int led_name, int flag)
{
    return ledctrl.flag_is_set(led_name, flag);
}

void led_clear_flag(int led_name, int flag)
{
    ledctrl.clear_flag(led_name, flag);
}

void led_set_color(enum INDICATOR_TYPES led_name, int led_color, int flags)
{
    ledctrl.set_color(led_name, led_color, flags);
}

void led_post(void)
{
    ledctrl.led_post();
}

void led_setup(void)
{
    ledctrl.led_setup();
}
