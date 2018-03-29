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

#include "TSL2591.h"

TSL2591::TSL2591 (I2C& tsl2591_i2c, uint8_t tsl2591_addr):
    _i2c(tsl2591_i2c), _addr(tsl2591_addr<<1)
{
    _init = false;
    _integ = TSL2591_INTT_100MS;
    _gain = TSL2591_GAIN_LOW;
}
/*
 *  Initialize TSL2591
 *  Checks ID and sets gain and integration time
 */
bool TSL2591::init(void)
{
    char write[] = {(TSL2591_CMD_BIT|TSL2591_REG_ID)};
    if(_i2c.write(_addr, write, 1, 0) == 0) {
        char read[1];
        _i2c.read(_addr, read, 1, 0);
        if(read[0] == TSL2591_ID) {
            _init = true;
            setGain(TSL2591_GAIN_LOW);
            setTime(TSL2591_INTT_100MS);
            disable();
            return true;
        }
    }
    return false;
}
/*
 *  Power On TSL2591
 */
void TSL2591::enable(void)
{
    char write[] = {(TSL2591_CMD_BIT|TSL2591_REG_ENABLE), (TSL2591_EN_PON|TSL2591_EN_AEN|TSL2591_EN_AIEN|TSL2591_EN_NPIEN)};
    _i2c.write(_addr, write, 2, 0);
}
/*
 *  Power Off TSL2591
 */
void TSL2591::disable(void)
{
    char write[] = {(TSL2591_CMD_BIT|TSL2591_REG_ENABLE), (TSL2591_EN_POFF)};
    _i2c.write(_addr, write, 2, 0);
}
/*
 *  Set Gain and Write
 *  Set gain and write time and gain
 */
void TSL2591::setGain(tsl2591Gain_t gain)
{
    enable();
    _gain = gain;
    char write[] = {(TSL2591_CMD_BIT|TSL2591_REG_CONTROL), (char)(_integ|_gain)};
    _i2c.write(_addr, write, 2, 0);
    disable();
}
/*
 *  Set Integration Time and Write
 *  Set gain and write time and gain
 */
void TSL2591::setTime(tsl2591IntegrationTime_t integ)
{
    enable();
    _integ = integ;
    char write[] = {(TSL2591_CMD_BIT|TSL2591_REG_CONTROL), (char)(_integ|_gain)};
    _i2c.write(_addr, write, 2, 0);
    disable();
}
/*
 *  Read ALS
 *  Read full spectrum, infrared, and visible
 */
void TSL2591::getALS(void)
{
    enable();
    for(uint8_t t=0; t<=_integ+1; t++) {
        wait(0.12);
    }
    char write1[] = {(TSL2591_CMD_BIT|TSL2591_REG_CHAN1_L)};
    _i2c.write(_addr, write1, 1, 0);
    char read1[2];
    _i2c.read(_addr, read1, 2, 0);
    char write2[] = {(TSL2591_CMD_BIT|TSL2591_REG_CHAN0_L)};
    _i2c.write(_addr, write2, 1, 0);
    char read2[2];
    _i2c.read(_addr, read2, 2, 0);
    rawALS = (((read1[1]<<8)|read1[0])<<16)|((read2[1]<<8)|read2[0]);
    disable();
    full = rawALS & 0xFFFF;
    ir = rawALS >> 16;
    visible = full - ir;
}
/*
 *  Calculate Lux
 */
void TSL2591::calcLux(void)
{
    float atime, again, cpl, lux1, lux2, lux3;
    if((full == 0xFFFF)|(ir == 0xFFFF)) {
        lux3 = 0;
        return;
    }
    switch(_integ) {
        case TSL2591_INTT_100MS:
            atime = 100.0F;
            break;
        case TSL2591_INTT_200MS:
            atime = 200.0F;
            break;
        case TSL2591_INTT_300MS:
            atime = 300.0F;
            break;
        case TSL2591_INTT_400MS:
            atime = 400.0F;
            break;
        case TSL2591_INTT_500MS:
            atime = 500.0F;
            break;
        case TSL2591_INTT_600MS:
            atime = 600.0F;
            break;
        default:
            atime = 100.0F;
            break;
    }
    switch(_gain) {
        case TSL2591_GAIN_LOW:
            again = 1.0F;
            break;
        case TSL2591_GAIN_MED:
            again = 25.0F;
            break;
        case TSL2591_GAIN_HIGH:
            again = 428.0F;
            break;
        case TSL2591_GAIN_MAX:
            again = 9876.0F;
            break;
        default:
            again = 1.0F;
            break;
    }
    cpl = (atime * again) / TSL2591_LUX_DF;
    lux1 = ((float)full - (TSL2591_LUX_COEFB * (float)ir)) / cpl;
    lux2 = (( TSL2591_LUX_COEFC * (float)full ) - ( TSL2591_LUX_COEFD * (float)ir)) / cpl;
    lux3 = lux1 > lux2 ? lux1 : lux2;
    lux = (uint32_t)lux3;
}
