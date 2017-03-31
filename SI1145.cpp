/***************************************************
 This is a library for the Si1145 UV/IR/Visible Light Sensor
 Designed specifically to work with the Si1145 sensor
 These sensors use I2C to communicate, 2 pins are required to
 interface
 Written by Herve Grabas.
 BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "SI1145.h"
#include <EEPROM.h> //In order to read/write from EEPROM

#define ALS_HISTORY_LENGTH           5 //Number of averages
static uint16_t alsDataHistory[ALS_HISTORY_LENGTH];

SI1145::SI1145() {
    _addr = SI1145_ADDR;
    Dark_Vis_0_Low = 256;
    Dark_Vis_1_Low = 256;
    Dark_Vis_2_Low = 256;
    Dark_Vis_3_Low = 256;
    Dark_Vis_4_Low = 256;
    Dark_Vis_5_Low = 256;
    Dark_Vis_6_Low = 256;
    Dark_Vis_7_Low = 256;
    Dark_Vis_0_High = 256;
    Dark_Vis_1_High = 256;
    Dark_Vis_2_High = 256;
    Dark_Vis_3_High = 256;
    Dark_Vis_4_High = 256;
    Dark_Vis_5_High = 256;
    Dark_Vis_6_High = 256;
    Dark_Vis_7_High  = 256;
    
    Dark_IR_0_Low = 256;
    Dark_IR_1_Low = 256;
    Dark_IR_2_Low = 256;
    Dark_IR_3_Low = 256;
    Dark_IR_4_Low = 256;
    Dark_IR_5_Low = 256;
    Dark_IR_6_Low = 256;
    Dark_IR_7_Low = 256;
    Dark_IR_0_High = 256;
    Dark_IR_1_High = 256;
    Dark_IR_2_High = 256;
    Dark_IR_3_High = 256;
    Dark_IR_4_High = 256;
    Dark_IR_5_High = 256;
    Dark_IR_6_High = 256;
    Dark_IR_7_High = 256;
    
    gainVis = 0;
    gainIR = 0;
    
    rangeVis = 0;
    rangeIR = 0;
}

static void Write_DarkFramesToEEPROM(uint8_t gain, uint8_t range, uint8_t sense, uint8_t value) {
    uint8_t addr = gain + range * 8 + value * 16;
    EEPROM.write(addr, value);
    EEPROM.commit();
}


static void Read_DarkFramesFromEEPROM() {
    
    
}

boolean SI1145::begin(void) {
    Wire.begin();
    
    uint8_t id = read8(SI1145_REG_PARTID);
    if (id != 0x45) return false; // look for SI1145
    
    reset();
    
    // Enable UVindex measurement coefficients!
    write8(SI1145_REG_UCOEFF0, 0x29);
    write8(SI1145_REG_UCOEFF1, 0x89);
    write8(SI1145_REG_UCOEFF2, 0x02);
    write8(SI1145_REG_UCOEFF3, 0x00);
    
    // Enable Temperature, VIS and IR
    writeParam(SI1145_PARAM_CHLIST,  SI1145_PARAM_CHLIST_ENAUX |
               SI1145_PARAM_CHLIST_ENALSIR | SI1145_PARAM_CHLIST_ENALSVIS);
    
    // Set the module in Forced mode
    // The parameter MEAS_RATE, when zero, places the internal sequencer in Forced Measurement mode.
    // Measurement rate
    write8(SI1145_REG_MEASRATE0, 0x00);
    write8(SI1145_REG_MEASRATE1, 0x00);
    
    // 1/. Write 0 to the command register = NOP command
    write8(SI1145_REG_COMMAND, SI1145_NOP);
    
    // 2/. Put the module in ALS force mode
    write8(SI1145_REG_COMMAND, SI1145_ALS_FORCE);
    
    // Disable the interrupt pin which unused
    // INT_OE controls the INT pin drive
    // 0: INT pin is never driven
    // 0 is the default value there is nothing to do.
    //write8(SI1145_REG_INTCFG, 0x00);
    
    // Enable interrupts status for ALS
    // Disable interrupt status for PS1, PS2, PS3
    // ALS_IE=1: Assert INT pin whenever VIS or UV measurements are ready
    write8(SI1145_REG_IRQEN, SI1145_REG_IRQEN_ALSEVERYSAMPLE);
    
    //Typically, the host software is expected to read the IRQ_STATUS register, stores a local copy, and then writes the same value back to the IRQ_STATUS to clear the interrupt source. The INT_CFG register is normally written with '1'.
    
    // Use the small IR diode
    writeParam(SI1145_PARAM_ALSIRADCMUX, SI1145_PARAM_ADCMUX_SMALLIR);
    // fastest clocks, clock div 1
    writeParam(SI1145_PARAM_ALSIRADCGAIN, 0);
    // take 511 clocks to measure
    writeParam(SI1145_PARAM_ALSIRADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
    // in high range mode
    writeParam(SI1145_PARAM_ALSIRADCMISC, SI1145_PARAM_ALSIRADCMISC_RANGE);
    
    // fastest clocks, clock div 1
    writeParam(SI1145_PARAM_ALSVISADCGAIN, 0);
    // take 511 clocks to measure
    writeParam(SI1145_PARAM_ALSVISADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
    // in high range mode (not normal signal)
    writeParam(SI1145_PARAM_ALSVISADCMISC, SI1145_PARAM_ALSVISADCMISC_VISRANGE);
    
    return true;
}

void SI1145::reset() {
    write8(SI1145_REG_MEASRATE0, 0);
    write8(SI1145_REG_MEASRATE1, 0);
    write8(SI1145_REG_IRQEN, 0);
    write8(SI1145_REG_IRQMODE1, 0);
    write8(SI1145_REG_IRQMODE2, 0);
    write8(SI1145_REG_INTCFG, 0);
    write8(SI1145_REG_IRQSTAT, 0xFF);
    
    write8(SI1145_REG_COMMAND, SI1145_RESET);
    delay(10);
    write8(SI1145_REG_HWKEY, 0x17);
    delay(10);
}


//////////////////////////////////////////////////////

// adjust the visible gain
void SI1145::setVisibleGain(uint8_t gain) {
    writeParam(SI1145_PARAM_ALSVISADCGAIN, gain);
}

// adjust the IR gain
void SI1145::setIRGain(uint8_t gain) {
    writeParam(SI1145_PARAM_ALSIRADCGAIN, gain);
}

// set the visible range
void SI1145::setVisibleRange(uint8_t range) {
    writeParam(SI1145_PARAM_ALSVISADCMISC, range );
}

// set the IR range
void SI1145::setIRRange(uint8_t range) {
    writeParam(SI1145_PARAM_ALSIRADCMISC, range);
}

//broken for now
// returns the UV index * 100 (divide by 100 to get the index)
//uint16_t SI1145::readUV(void) {
//    return read16(0x2C);
//}

// increase or decrease the gain based on the readings
void SI1145::autoRange(uint16_t _vis, uint16_t _ir) {
    
    // for the visible light
    if (_vis > 25000) { //overflow in visible light or getting close to saturation (saturation happens at 32767 acoording to AN498
        if (gainVis == 0) { // At the lowest gain and saturation
            if (!rangeVis) { // Not in high range mode
                rangeVis = SI1145_PARAM_ALSVISADCMISC_VISRANGE;
            }
        } //Else we are in high range mode can't decrease the range any further
        else if (gainVis > 0){ // Not at the lowest gain setting
            gainVis -= 1;
        }
    }
    else if (_vis < 1500) { //no overflow and low readings <1000
        if (gainVis < 7) { //if the gain is not maximum increase it
            //setVisibleGain(gainVis+(uint8_t)1);
            gainVis += 1;
        }
        else if (rangeVis) { //if the gain is max and range high decrease the range
            rangeVis = SI1145_PARAM_ALSVISADCMISC_VISRANGE_LOW;
            gainVis = 0; //Decrease the gain by factor of 8 because range is x14.5 -- removed too dangerous sometime gain is not properly read over i2c.
        }
    }
    // for the IR light
    if (_ir > 25000) { //overflow in IR light or getting close to saturation (saturation happens at 0x7FFF acoording to AN498
        if (gainIR == 0) { // At the lowest gain and saturation
            if (!rangeIR) { // Not in high range mode
                rangeIR = SI1145_PARAM_ALSIRADCMISC_RANGE;
            }
        } //Else we are in high range mode can't decrease the range any further
        else if (gainIR > 0) { // Not at the lowest gain setting
            gainIR -= 1;
        }
    }
    else if (_ir < 1500) { //no overflow and low readings
        if (gainIR < 7) { //if the gain is not maximum increase it
            gainIR += 1;
        }
        else if (rangeIR) { //if the gain is max and range high decrease the range
            rangeIR = SI1145_PARAM_ALSIRADCMISC_RANGE_LOW;
            gainIR += 0; //Decrease the gain by factor of 8 because range is x14.5
        }
    }
    setVisibleGain(gainVis);
    setIRGain(gainIR);
    setVisibleRange(rangeVis);
    setIRRange(rangeIR);
}

// Force an ALS measurment
float SI1145::forceMeasLux(void){
    float _vis;
    float _ir;
    uint16_t vis;
    uint16_t ir;
    //sending and ALS force command
    write8(SI1145_REG_COMMAND, SI1145_ALS_FORCE);
    //then we should wait until the measurments completes
    //the worste case is 3*25.55x2^7 = 3*3.28ms = 9.84ms
    delay(20);
    //make sure that there is no overflow
    uint8_t resp = read8(SI1145_REG_RESPONSE);
    if ((resp & 0x80) != 0) {
        // we have an overflow
        // we clear the overflow
        write8(SI1145_REG_COMMAND, SI1145_NOP);
        vis = 0x7FFF; //put the default max value
        ir  = 0x7FFF; //put the default max value
    }
    else {
        vis = readVisible();
        ir  = readIR();
    }
    uint16_t tp  = readTemp();
    
    //clearing the IRQ status register
    write8(SI1145_REG_IRQSTAT,0xFF);
    
    //compensate for temperature drifts
    //temperature sensor offset 11136 at 25C (reference)
    //35 ADC count per C
    switch (gainVis) {
        case Gain_0:
            _vis = vis-0.3f*(tp-11136)/35.0f;
            break;
        case Gain_1:
            _vis = vis-0.11f*(tp-11136)/35.0f;
            break;
        case Gain_2:
            _vis = vis-0.06f*(tp-11136)/35.0f;
            break;
        case Gain_3:
            _vis = vis-0.03f*(tp-11136)/35.0f;
            break;
        case Gain_4:
            _vis = vis-0.01f*(tp-11136)/35.0f;
            break;
        case Gain_5:
            _vis = vis-0.008f*(tp-11136)/35.0f;
            break;
        case Gain_6:
            _vis = vis-0.007f*(tp-11136)/35.0f;
            break;
        case Gain_7:
            _vis = vis-0.008f*(tp-11136)/35.0f;
            break;
    }
    switch (gainIR) {
        case Gain_0:
            _ir = ir-0.3f*(tp-11136)/35.0f;
            break;
        case Gain_1:
            _ir = ir-0.06f*(tp-11136)/35.0f;
            break;
        case Gain_2:
            _ir = ir-0.03f*(tp-11136)/35.0f;
            break;
        case Gain_3:
            _ir = ir-0.01f*(tp-11136)/35.0f;
            break;
    }
    
    //compute the lux
    float _rangeVis = 1 + 13.5f * rangeVis; //14.5 if high range, 1 otherwise
    float _rangeIR  = 1 + 13.5f * rangeIR; //14.5 if high range, 1 otherwise
    float lux = (5.41f * vis * _rangeVis) / (1 << gainVis) + (-0.08f * ir * _rangeIR) / (1 << gainIR);
    if (lux < 0)
        lux = 0.0;
    
    // autorange the sensor
    autoRange(vis, ir);
    return lux;
}

// returns visible light levels & calibrate dark frames
uint16_t SI1145::readVisible(void) {
    uint16_t vis = read8(SI1145_REG_ALSVISDATA0);
    vis |= (read8(SI1145_REG_ALSVISDATA1) <<8);
    if ((vis > 0) && (vis < 256)) {
    switch (rangeVis) {
        case Range_0:
        switch (gainVis) {
            case Gain_0:
                if (vis < Dark_Vis_0_Low)
                    Dark_Vis_0_Low = vis;
                break;
            case Gain_1:
                if (vis < Dark_Vis_1_Low)
                    Dark_Vis_1_Low = vis;
                break;
            case Gain_2:
                if (vis < Dark_Vis_2_Low)
                    Dark_Vis_2_Low = vis;
                break;
            case Gain_3:
                if (vis < Dark_Vis_3_Low)
                    Dark_Vis_3_Low = vis;
                break;
            case Gain_4:
                if (vis < Dark_Vis_4_Low)
                    Dark_Vis_4_Low = vis;
                break;
            case Gain_5:
                if (vis < Dark_Vis_5_Low)
                    Dark_Vis_5_Low = vis;
                break;
            case Gain_6:
                if (vis < Dark_Vis_6_Low)
                    Dark_Vis_6_Low = vis;
                break;
            case Gain_7:
                if (vis < Dark_Vis_7_Low)
                    Dark_Vis_7_Low = vis;
                break;}
        break;
        case Range_1:
            switch (gainVis) {
                case Gain_0:
                    if (vis < Dark_Vis_0_High)
                        Dark_Vis_0_High = vis;
                    break;
                case Gain_1:
                    if (vis < Dark_Vis_1_High)
                        Dark_Vis_1_High = vis;
                    break;
                case Gain_2:
                    if (vis < Dark_Vis_2_High)
                        Dark_Vis_2_High = vis;
                    break;
                case Gain_3:
                    if (vis < Dark_Vis_3_High)
                        Dark_Vis_3_High = vis;
                    break;
                case Gain_4:
                    if (vis < Dark_Vis_4_High)
                        Dark_Vis_4_High = vis;
                    break;
                case Gain_5:
                    if (vis < Dark_Vis_5_High)
                        Dark_Vis_5_High = vis;
                    break;
                case Gain_6:
                    if (vis < Dark_Vis_6_High)
                        Dark_Vis_6_High = vis;
                    break;
                case Gain_7:
                    if (vis < Dark_Vis_7_High)
                        Dark_Vis_7_High = vis;
                break;}
        break;}}
    return vis;
}

// returns IR light levels & calibrate dark frames
uint16_t SI1145::readIR(void) {
    uint16_t ir = read8(SI1145_REG_ALSIRDATA0);
    ir |= (read8(SI1145_REG_ALSIRDATA1) <<8);
    if ((ir > 0) && (ir < 256)) {
    switch (rangeIR) {
        case Range_0:
            switch (gainIR) {
                case Gain_0:
                    if (ir < Dark_IR_0_Low)
                        Dark_IR_0_Low = ir;
                    break;
                case Gain_1:
                    if (ir < Dark_IR_1_Low)
                        Dark_IR_1_Low = ir;
                    break;
                case Gain_2:
                    if (ir < Dark_IR_2_Low)
                        Dark_IR_2_Low = ir;
                    break;
                case Gain_3:
                    if (ir < Dark_IR_3_Low)
                        Dark_IR_3_Low = ir;
                    break;
                case Gain_4:
                    if (ir < Dark_IR_4_Low)
                        Dark_IR_4_Low = ir;
                    break;
                case Gain_5:
                    if (ir < Dark_IR_5_Low)
                        Dark_IR_5_Low = ir;
                    break;
                case Gain_6:
                    if (ir < Dark_IR_6_Low)
                        Dark_IR_6_Low = ir;
                    break;
                case Gain_7:
                    if (ir < Dark_IR_7_Low)
                        Dark_IR_7_Low = ir;
                break;}
            break;
        case Range_1:
            switch (gainIR) {
                case Gain_0:
                    if (ir < Dark_IR_0_High)
                        Dark_IR_0_High = ir;
                    break;
                case Gain_1:
                    if (ir < Dark_IR_1_High)
                        Dark_IR_1_High = ir;
                    break;
                case Gain_2:
                    if (ir < Dark_IR_2_High)
                        Dark_IR_2_High = ir;
                    break;
                case Gain_3:
                    if (ir < Dark_IR_3_High)
                        Dark_IR_3_High = ir;
                    break;
                case Gain_4:
                    if (ir < Dark_IR_4_High)
                        Dark_IR_4_High = ir;
                    break;
                case Gain_5:
                    if (ir < Dark_IR_5_High)
                        Dark_IR_5_High = ir;
                    break;
                case Gain_6:
                    if (ir < Dark_IR_6_High)
                        Dark_IR_6_High = ir;
                    break;
                case Gain_7:
                    if (ir < Dark_IR_7_High)
                        Dark_IR_7_High = ir;
                break;}
        break;}}
    return ir;
}


// returns the temperature measurment
uint16_t SI1145::readTemp(void) {
    uint16_t temp = read8(SI1145_REG_UVINDEX0);
    temp |= (read8(SI1145_REG_UVINDEX1) <<8);
    return temp;
}

// returns "Proximity" - assumes an IR LED is attached to LED
uint16_t SI1145::readProx(void) {
    return read16(SI1145_REG_PS1DATA0);
}

/*********************************************************************/

uint8_t SI1145::writeParam(uint8_t p, uint8_t v) {
    //Serial.print("Param 0x"); Serial.print(p, HEX);
    //Serial.print(" = 0x"); Serial.println(v, HEX);
    
    write8(SI1145_REG_PARAMWR, v);
    write8(SI1145_REG_COMMAND, p | SI1145_PARAM_SET);
    return read8(SI1145_REG_PARAMRD);
}

uint8_t SI1145::readParam(uint8_t p) {
    write8(SI1145_REG_COMMAND, p | SI1145_PARAM_QUERY);
    return read8(SI1145_REG_PARAMRD);
}

/*********************************************************************/

uint8_t  SI1145::read8(uint8_t reg) {
    uint16_t val;
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)reg | 0x40); //to disable auto-increment
    Wire.endTransmission();
    
    Wire.requestFrom((uint8_t)_addr, (uint8_t)1);
    return Wire.read();
}

uint16_t SI1145::read16(uint8_t a) {
    uint16_t ret;
    
    Wire.beginTransmission(_addr); // start transmission to device
    Wire.write(a); // sends register address to read from
    Wire.endTransmission(); // end transmission
    
    Wire.requestFrom(_addr, (uint8_t)2);// send data n-bytes read
    ret = Wire.read(); // receive DATA
    ret |= (uint16_t)Wire.read() << 8; // receive DATA
    
    return ret;
}

void SI1145::write8(uint8_t reg, uint8_t val) {
    
    Wire.beginTransmission(_addr); // start transmission to device 
    Wire.write(reg); // sends register address to write
    Wire.write(val); // sends value
    Wire.endTransmission(); // end transmission
}
