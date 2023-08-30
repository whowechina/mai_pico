/*
 * Copyright (c) 2021-2022 Antonio González
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mpr121.h"

void mpr121_init(i2c_inst_t *i2c_port, uint8_t i2c_addr,
                 mpr121_sensor_t *sensor) {
    sensor->i2c_port = i2c_port;
    sensor->i2c_addr = i2c_addr;
    
    // Enter stop mode by setting ELEPROX_EN and ELE_EN bits to zero.
    // This is needed because register write operations can only take
    // place in stop mode.
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);

    // Writing 0x80 (SOFT_RESET) with 0x63 asserts soft reset.
    mpr121_write(MPR121_SOFT_RESET_REG, 0x63, sensor);
    
    // == Capacitance sensing settings (AN2889), Filtering and =========
    //    timing settings (AN3890)

    // These settings are configured in two registers: the Filter and
    // Global CDC CDT Configuration registers (0x5C, 0x5D).
    //
    // Charge-discharge current (CDC) and charge-discharge time (CDT) 
    // can be configured globally or on a per-electrode basis. Here,
    // The global CDC and CDT values are set to their defaults, and then
    // these values are overriden by independently configuring each
    // electrode (auto-configuration).

    // Filter/global CDC configuration register (0x5C)
    //
    // First filter iterations (FFI), bits 7-6. Number of samples taken
    // as input to the first level of filtering. Default is 0b00 (sets
    // samples taken to 6)
    //
    // Charge-discharge current (CDC), bits 5-0. Sets the value of
    // charge-discharge current applied to the electrode. Max is 63 µA
    // in 1 µA steps. Default is 0b010000 (16 µA)
    //
    // AFE configuration register default, 0b00010000 = 0x10
    mpr121_write(MPR121_AFE_CONFIG_REG, 0x10, sensor);

    // Filter/global CDC configuration register (0x5D)
    //
    // Charge discharge time (CDT), bits 7-5. Selects the global value
    // of charge time applied to electrode. The maximum is 32 μs,
    // programmable as 2^(n-2) μs. Default is 0b001 (time is set to
    // 0.5 µs)
    //
    // Second filter iterations (SFI), bits 4-3. Selects the number of
    // samples taken for the second level filter. Default is 0b00
    // (number of samples is set to 4)
    //
    // Electrode sample interval (ESI), bits 2-0. Controls the sampling
    // rate of the device. The maximum is 128 ms, programmable to 2^n
    // ms. Decrease this value for better response time, increase to
    // save power. Default is 0b100 (period set to 16 ms).
    // 
    // Filter configuration register default, 0b00100100 = 0x24
    // I do not need power saving features but I want fast responses,
    // so I set this to 0x20.
    mpr121_write(MPR121_FILTER_CONFIG_REG, 0x20, sensor);

    // Auto-configuration
    //
    // Sets automatically charge current (CDC) and time (CDT) values for
    // each electrode.
    //
    // Autoconfig USL register: the upper limit for the
    // auto-configuration. This value (and those that follow below)
    // were calculated based on Vdd = 3.3 V and following the equations
    // in NXP Application Note AN3889.
    // USL = 201 = 0xC9
    mpr121_write(MPR121_AUTOCONFIG_USL_REG, 0xC9, sensor);

    // Autoconfig target level register: the target level for the 
    // auto-configuration baseline search.
    // TL = 181 = 0xB5
    mpr121_write(MPR121_AUTOCONFIG_TARGET_REG, 0xB5, sensor);

    // Autoconfig LSL register: the lower limit for the
    // auto-configuration.
    // LSL = 131 = 0x83
    mpr121_write(MPR121_AUTOCONFIG_LSL_REG, 0x83, sensor);

    // Autoconfiguration control register. Default value is 0b00001011 =
    // 0x0B, where:
    //
    // First filter iterations (FFI), bits 7-6. Must be the same value
    // of FFI as in register MPR121_AFE_CONFIG_REG (0x5C) above;
    // default is 0b00.
    //
    // Retry, bits 5-4. Default is disabled, 0b00.
    //
    // Baseline value adjust (BVA), bits 3-2. This value must be the
    // same as the CL (calibration lock) value in the Electrode
    // Configuration Register, below, i.e. 0b10.
    //
    // Automatic Reconfiguration Enable (ARE), bit 1. Default is 0b1,
    // enabled.
    //
    // Automatic Reconfiguration Enable (ACE), bit 0. Default is 0b1,
    // enabled.
    mpr121_write(MPR121_AUTOCONFIG_CONTROL_0_REG, 0x0B, sensor);

    // == Baseline system (AN3891) =====================================

    // Maximum Half Delta (MHD): Determines the largest magnitude of
    // variation to pass through the baseline filter. The range of the
    // effective value is 1~63.
    mpr121_write(MPR121_MAX_HALF_DELTA_RISING_REG, 0x01, sensor);
    mpr121_write(MPR121_MAX_HALF_DELTA_FALLING_REG, 0x01, sensor);
    
    // Noise Half Delta (NHD): Determines the incremental change when
    // non-noise drift is detected. The range of the effective value is
    // 1~63.
    mpr121_write(MPR121_NOISE_HALF_DELTA_RISING_REG, 0x01, sensor);
    mpr121_write(MPR121_NOISE_HALF_DELTA_FALLING_REG, 0x01, sensor);
    mpr121_write(MPR121_NOISE_HALF_DELTA_TOUCHED_REG, 0x01, sensor);  
    
    // Noise Count Limit (NCL): Determines the number of samples
    // consecutively greater than the Max Half Delta value. This is
    // necessary to determine that it is not noise. The range of the
    // effective value is 0~255.
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_RISING_REG, 0x00, sensor);
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_FALLING_REG, 0xFF, sensor);
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_TOUCHED_REG, 0x00, sensor);
    
    // Filter Delay Count Limit (FDL): Determines the operation rate of
    // the filter. A larger count limit means the filter delay is
    // operating more slowly. The range of the effective value is 0~255.
    mpr121_write(MPR121_FILTER_DELAY_COUNT_RISING_REG, 0x00, sensor);
    mpr121_write(MPR121_FILTER_DELAY_COUNT_FALLING_REG, 0x02, sensor);
    mpr121_write(MPR121_FILTER_DELAY_COUNT_TOUCHED_REG, 0x00, sensor);

    // == Debounce and thresholds (AN3892) =============================
    
    // Debounce. Value range for each is 0~7.
    // Bits 2-0, debounce touch (DT).
    // Bits 6-4, debounce release (DR).
    mpr121_write(MPR121_DEBOUNCE_REG, 0x00, sensor);

    // Touch and release threshold values for all electrodes.
    for (uint8_t i=0; i<12; i++) {
        mpr121_write(MPR121_TOUCH_THRESHOLD_REG + i * 2, 0x0F, sensor);
        mpr121_write(MPR121_RELEASE_THRESHOLD_REG + i * 2, 0x0A, sensor);
    }

    // Electrode Configuration Register (ECR, 0x5E). This must be the
    // last register to write to because setting ELEPROX_EN and/or
    // ELE_EN to non-zero puts the sensor in Run Mode.
    //
    // Calibration lock (CL), bits 7-6. The default on reset is 0b00
    // (CL enabled). Here I set this instead to 0b10 because this
    // enables baseline tracking with initial baseline value loaded
    // with the 5 high bits of the first electrode data value, which
    // makes the sensor stabilise sooner. Note that ths value must
    // match BVA bits in the Auto-configure Control Register above.
    //
    // Proximity enable (ELEPROX_EN), bits 5-4. Default, 0b00
    // (proximity detection disabled).
    //
    // Electrode enabled (ELE_EN), bits 3-0. Default, 0b1100 (enable
    // all 12 electrodes).
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x8C, sensor);
}