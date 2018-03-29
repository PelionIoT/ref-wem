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

#ifndef __MULTIADDRLCD_H__
#define __MULTIADDRLCD_H__

#include <TextLCD.h>

// LCD which can have I2C slave address of eith 0x4e or 0x7e
class MultiAddrLCD {
public:

    MultiAddrLCD(PinName rs, PinName r, PinName d4, PinName d5, PinName d6, PinName d7);


    /*Only supporting 16x2 LCDs, so string will be truncated at 32
      characters.*/
    int printf(const char *format, ...);
    /*Print on the given line (0 or 1)*/
    int printline(int line, const char *msg);
    int printlinef(int line, const char *format, ...);
    void setBacklight(TextLCD_Base::LCDBacklight mode);
    void setCursor(TextLCD_Base::LCDCursor mode);
    void setUDC(unsigned char c, char *udc_data);

    void locate(int column, int row);
    void putc(int c);

private:
    TextLCD _lcd1;
};

#endif
