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

#include "multiaddrlcd.h"

MultiAddrLCD::MultiAddrLCD(PinName rs, PinName e, PinName d4, PinName d5, PinName d6, PinName d7)
    : _lcd1(rs, e, d4, d5, d6, d7)
{
}

/*Only supporting 16x2 LCDs, so string will be truncated at 32
  characters.*/
int MultiAddrLCD::printf(const char *format, ...)
{
    int rc;
    char buf[33];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    rc = _lcd1.printf(buf);
    return rc;
}

/*Only supporting 16x2 LCDs, so string will be truncated at 16
  characters.*/
int MultiAddrLCD::printline(int line, const char *msg)
{
    int rc;
    char buf[17];
    snprintf(buf, sizeof(buf), "%s", msg);
    _lcd1.locate(0, line);
    rc = _lcd1.printf("%-16s", buf);

    return rc;
}

int MultiAddrLCD::printlinef(int line, const char *format, ...)
{
    int rc;
    char buf[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    _lcd1.locate(0, line);
    rc = _lcd1.printf("%-16s", buf);

    return rc;
}

void MultiAddrLCD::setBacklight(TextLCD_Base::LCDBacklight mode)
{
    _lcd1.setBacklight(mode);
}

void MultiAddrLCD::setCursor(TextLCD_Base::LCDCursor mode)
{
    _lcd1.setCursor(mode);
}

void MultiAddrLCD::setUDC(unsigned char c, char *udc_data)
{
    _lcd1.setUDC(c, udc_data);
}

void MultiAddrLCD::locate(int column, int row)
{
    _lcd1.locate(column, row);
}

void MultiAddrLCD::putc(int c)
{
    _lcd1.putc(c);
}
