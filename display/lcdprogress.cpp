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

#include "lcdprogress.h"

LCDProgress::LCDProgress(MultiAddrLCD &lcd)
    : _lcd(lcd), _buffer(""), _previous("")
{
    char backslash[] = {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00};
    lcd.setUDC(7, backslash);
    uint8_t pixel_row[] = {0x0, 0x10, 0x18, 0x1C, 0x1E};
    for (int i = 1; i < 5; i++) {
        char pixels[8];
        for (int j = 0; j < 8; j++) {
            pixels[j] = pixel_row[i];
        }
        _lcd.setUDC(i, pixels);
    }
}

void LCDProgress::set_progress(const std::string &message, uint32_t progress,
                               uint32_t total)
{
    static int spinner_counter = 0;
    char spinner;
    std::string progressbar(16, ' ');
    uint32_t i, lines, bars;

    switch (spinner_counter) {
        case 0:
            spinner = '-';
            break;
        case 1:
            spinner = '/';
            break;
        case 2:
            spinner = '|';
            break;
        case 3:
            spinner = 7;
            break;
        default:
            spinner = '?';
            break;
    }

    /* calculate progress bar */
    lines = progress * 16 * 5 / total;
    bars = lines / 5;
    for (i = 0; i < bars; i++) {
        progressbar[i] = 0xff; // Full bar
    }
    if (i < 16 && lines > bars * 5) {
        uint8_t partial_columns = lines % 5;
        progressbar[i] = partial_columns;
        i++;
    }
    for (; i < 16; i++) {
        progressbar[i] = ' ';
    }

    char buffer[33];
    snprintf(buffer, 33, "%-15s%c%s", message.c_str(), spinner,
             progressbar.c_str());
    _buffer = buffer;

    spinner_counter = (spinner_counter + 1) % 4;
}

void LCDProgress::refresh()
{
    if (_previous.length() == 0) {
        /* No previous state, splat the whole thing. */
        _lcd.printf("%s", _buffer.c_str());
    } else {
        /* Only change the characters that need changing to speed up update. */
        for (int i = 0; i < 32; i++) {
            if (_buffer[i] != _previous[i]) {
                _lcd.locate(i % 16, i / 16);
                _lcd.putc(_buffer[i]);
            }
        }
    }
    _previous = _buffer;
}

void LCDProgress::reset() { _previous.clear(); }