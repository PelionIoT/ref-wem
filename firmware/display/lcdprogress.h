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

#ifndef __LCDPROGRESS_H_
#define __LCDPROGRESS_H_

#include "multiaddrlcd.h"

#include <string>

class LCDProgress {
public:
    LCDProgress(MultiAddrLCD &lcd);
    void set_progress(const std::string &message, uint32_t progress,
                      uint32_t total);
    void refresh();
    void reset();

private:
    MultiAddrLCD &_lcd;
    std::string _buffer;
    std::string _previous;
};

#endif
