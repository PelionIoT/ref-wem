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

#ifndef TSL2591_H
#define TSL2591_H

#include "mbed.h"

#define TSL2591_ADDR        (0x29)
#define TSL2591_ID          (0x50)

#define TSL2591_CMD_BIT     (0xA0)

#define TSL2591_EN_NPIEN    (0x80)
#define TSL2591_EN_SAI      (0x40)
#define TSL2591_EN_AIEN     (0x10)
#define TSL2591_EN_AEN      (0x02)
#define TSL2591_EN_PON      (0x01)
#define TSL2591_EN_POFF     (0x00)

#define TSL2591_LUX_DF      (408.0F)
#define TSL2591_LUX_COEFB   (1.64F)  // CH0 coefficient 
#define TSL2591_LUX_COEFC   (0.59F)  // CH1 coefficient A
#define TSL2591_LUX_COEFD   (0.86F)  // CH2 coefficient B

enum {
    TSL2591_REG_ENABLE          = 0x00,
    TSL2591_REG_CONTROL         = 0x01,
    TSL2591_REG_THRES_AILTL     = 0x04,
    TSL2591_REG_THRES_AILTH     = 0x05,
    TSL2591_REG_THRES_AIHTL     = 0x06,
    TSL2591_REG_THRES_AIHTH     = 0x07,
    TSL2591_REG_THRES_NPAILTL   = 0x08,
    TSL2591_REG_THRES_NPAILTH   = 0x09,
    TSL2591_REG_THRES_NPAIHTL   = 0x0A,
    TSL2591_REG_THRES_NPAIHTH   = 0x0B,
    TSL2591_REG_PERSIST         = 0x0C,
    TSL2591_REG_PID             = 0x11,
    TSL2591_REG_ID              = 0x12,
    TSL2591_REG_STATUS          = 0x13,
    TSL2591_REG_CHAN0_L         = 0x14,
    TSL2591_REG_CHAN0_H         = 0x15,
    TSL2591_REG_CHAN1_L         = 0x16,
    TSL2591_REG_CHAN1_H         = 0x17,
};

typedef enum {
    TSL2591_GAIN_LOW    = 0x00,
    TSL2591_GAIN_MED    = 0x01,
    TSL2591_GAIN_HIGH   = 0x02,
    TSL2591_GAIN_MAX    = 0x03,
} tsl2591Gain_t;

typedef enum {
    TSL2591_INTT_100MS  = 0x00,
    TSL2591_INTT_200MS  = 0x01,
    TSL2591_INTT_300MS  = 0x02,
    TSL2591_INTT_400MS  = 0x03,
    TSL2591_INTT_500MS  = 0x04,
    TSL2591_INTT_600MS  = 0x05,
} tsl2591IntegrationTime_t;

typedef enum {
    TSL2591_PER_EVERY   = 0x00,
    TSL2591_PER_ANY     = 0x01,
    TSL2591_PER_2       = 0x02,
    TSL2591_PER_3       = 0x03,
    TSL2591_PER_5       = 0x04,
    TSL2591_PER_10      = 0x05,
    TSL2591_PER_15      = 0x06,
    TSL2591_PER_20      = 0x07,
    TSL2591_PER_25      = 0x08,
    TSL2591_PER_30      = 0x09,
    TSL2591_PER_35      = 0x0A,
    TSL2591_PER_40      = 0x0B,
    TSL2591_PER_45      = 0x0C,
    TSL2591_PER_50      = 0x0D,
    TSL2591_PER_55      = 0x0E,
    TSL2591_PER_60      = 0x0F,
} tsl2591Persist_t;

class TSL2591
{
    public:
    TSL2591(I2C& tsl2591_i2c, uint8_t tsl2591_addr=TSL2591_ADDR);
    bool init(void);
    void enable(void);
    void disable(void);
    void setGain(tsl2591Gain_t gain);
    void setTime(tsl2591IntegrationTime_t integ);
    void getALS(void);
    void calcLux(void);
    volatile uint32_t           rawALS;
    volatile uint16_t           ir;
    volatile uint16_t           full;
    volatile uint16_t           visible;
    volatile uint32_t           lux;
    
    protected:
    I2C                         &_i2c;
    uint8_t                     _addr;
    bool                        _init;
    tsl2591Gain_t               _gain;
    tsl2591IntegrationTime_t    _integ;
};

#endif