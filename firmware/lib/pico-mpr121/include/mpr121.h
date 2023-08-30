/*
 * Copyright (c) 2021-2022 Antonio González
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MPR121_H_
#define _MPR121_H_

#include "pico.h"
#include "hardware/i2c.h"

/** \file mpr121.h
 * \brief Library for using an MPR121-based touch sensor with the
 * Raspberry Pi Pico
 *
*/

typedef struct mpr121_sensor {
    i2c_inst_t *i2c_port;
    uint8_t i2c_addr;
    // uint8_t i2c_sda;
    // uint8_t ic2_scl;
} mpr121_sensor_t;

/*! \brief MPR121 register map
 */
enum mpr121_register {
    MPR121_TOUCH_STATUS_REG = 0x00u,
    MPR121_OUT_OF_RANGE_STATUS_0_REG = 0x02u,
    MPR121_OUT_OF_RANGE_STATUS_1_REG = 0x03u,
    MPR121_ELECTRODE_FILTERED_DATA_REG = 0x04u,
    MPR121_BASELINE_VALUE_REG = 0x1Eu,
    // Registers 0x2B ~ 0x7F are control and configuration registers
    MPR121_MAX_HALF_DELTA_RISING_REG = 0x2Bu,
    MPR121_NOISE_HALF_DELTA_RISING_REG = 0x2Cu,
    MPR121_NOISE_COUNT_LIMIT_RISING_REG = 0x2Du,
    MPR121_FILTER_DELAY_COUNT_RISING_REG = 0x2Eu,
    MPR121_MAX_HALF_DELTA_FALLING_REG = 0x2Fu,
    MPR121_NOISE_HALF_DELTA_FALLING_REG = 0x30u,
    MPR121_NOISE_COUNT_LIMIT_FALLING_REG = 0x31u,
    MPR121_FILTER_DELAY_COUNT_FALLING_REG = 0x32u,
    MPR121_NOISE_HALF_DELTA_TOUCHED_REG = 0x33u,
    MPR121_NOISE_COUNT_LIMIT_TOUCHED_REG = 0x34u,
    MPR121_FILTER_DELAY_COUNT_TOUCHED_REG = 0x35u,
    // (ELEPROX 0x36 .. 0x40)
    MPR121_TOUCH_THRESHOLD_REG = 0x41u,
    MPR121_RELEASE_THRESHOLD_REG = 0x42u,
    // (ELEPROX 0x59 .. 0x5A)
    MPR121_DEBOUNCE_REG = 0x5Bu,
    MPR121_AFE_CONFIG_REG = 0x5Cu,
    MPR121_FILTER_CONFIG_REG = 0x5Du,
    MPR121_ELECTRODE_CONFIG_REG = 0x5Eu,
    MPR121_ELECTRODE_CURRENT_REG = 0x5Fu,
    MPR121_ELECTRODE_CHARGE_TIME_REG = 0x6Cu,
    MPR121_GPIO_CTRL_0_REG = 0x73u,
    MPR121_GPIO_CTRL_1_REG = 0x74u,
    MPR121_GPIO_DATA_REG = 0x75u,
    MPR121_GPIO_DIRECTION_REG = 0x76u,
    MPR121_GPIO_ENABLE_REG = 0x77u,
    MPR121_GPIO_DATA_SET_REG = 0x78u,
    MPR121_GPIO_DATA_CLEAR_REG = 0x79u,
    MPR121_GPIO_DATA_TOGGLE_REG = 0x7Au,
    MPR121_AUTOCONFIG_CONTROL_0_REG = 0x7Bu,
    MPR121_AUTOCONFIG_CONTROL_1_REG = 0x7Cu,
    MPR121_AUTOCONFIG_USL_REG = 0x7Du,
    MPR121_AUTOCONFIG_LSL_REG = 0x7Eu,
    MPR121_AUTOCONFIG_TARGET_REG = 0x7Fu,
    MPR121_SOFT_RESET_REG = 0x80u
};

/*! \brief Initialise the MPR121 and configure registers
 *
 * The default parameters used here to configure the sensor are as in
 * the MPR121 Quick Start Guide (AN3944).
 * 
 * \param i2c_port The I2C instance, either i2c0 or i2c1
 * \param i2c_addr The I2C address of the MPR121 device
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
void mpr121_init(i2c_inst_t *i2c_port, uint8_t i2c_addr,
                 mpr121_sensor_t *sensor);

/*! \brief Write a value to the specified register
 *
 * \param reg The register address
 * \param val The value to write
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_write(enum mpr121_register reg, uint8_t val,
                         mpr121_sensor_t *sensor) {
    uint8_t buf[] = {reg, val};
    i2c_write_blocking(sensor->i2c_port, sensor->i2c_addr, buf, 2,
                       false);
}

/*! \brief Read a byte from the specified register
 *
 * \param reg The register address
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_read(enum mpr121_register reg, uint8_t *dst,
                        mpr121_sensor_t *sensor) {
    i2c_write_blocking(sensor->i2c_port, sensor->i2c_addr, &reg, 1,
                       true);
    i2c_read_blocking(sensor->i2c_port, sensor->i2c_addr, dst, 1,
                      false);
}

/*! \brief Read a 2-byte value from the specified register
 *
 * \param reg The register address
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_read16(enum mpr121_register reg, uint16_t *dst,
                          mpr121_sensor_t *sensor) {
    uint8_t vals[2];
    i2c_write_blocking(sensor->i2c_port, sensor->i2c_addr, &reg, 1,
                       true);
    i2c_read_blocking(sensor->i2c_port, sensor->i2c_addr, vals, 2,
                      false);
    *dst = vals[1] << 8 | vals[0];
}

/*! \brief Set touch and release thresholds
 * 
 * From the MPR121 datasheet (section 5.6):
 * > In a typical application, touch threshold is in the range 4--16,
 * > and it is several counts larger than the release threshold. This
 * > is to provide hysteresis and to prevent noise and jitter.
 *  
 * \param touch Touch threshold in the range 0--255
 * \param release Release threshold in the range 0--255
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_set_thresholds(uint8_t touch, uint8_t release,
                                  mpr121_sensor_t *sensor) {
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);
    if (config != 0){
        // Stop mode
        mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    }
    
    for (uint8_t i=0; i<12; i++) {
        mpr121_write(MPR121_TOUCH_THRESHOLD_REG + i * 2, touch, sensor);
        mpr121_write(MPR121_RELEASE_THRESHOLD_REG + i * 2, release,
            sensor);
    }

    if (config != 0){
        mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
    }
}

/*! \brief Enable only the number of electrodes specified
 * 
 * \param nelec Number of electrodes to enable
 * \param sensor Pointer to the structure that stores the MPR121 info
 * 
 * E.g. if `nelec` is 3, electrodes 0 to 2 will be enabled; if `nelec`
 * is 6, electrodes 0 to 5 will be enabled. From the datasheet:
 * "Enabling specific channels will save the scan time and sensing
 * field power spent on the unused channels."
 */
static void mpr121_enable_electrodes(uint8_t nelec,
                                     mpr121_sensor_t *sensor){
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);

    // Clear bits 3-0, which controls the operation of the 12
    // electrodes.
    config &= ~0x0f;

    // Set number of electrodes enabled
    config |= nelec;

    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
}

/*! \brief Read the touch/release status of all 13 input channels
 *
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 *
 * In the value read, bits 11-0 represent electrodes 11 to 0,
 * respectively, and bit 12 is the proximity detection channel. Each
 * bit represent the status of these channels: 1 if the channel is
 * touched, 0 if it is released.
 */
static void mpr121_touched(uint16_t *dst, mpr121_sensor_t *sensor) {
    mpr121_read16(MPR121_TOUCH_STATUS_REG, dst, sensor);
    *dst &= 0x0fff;
}

/*! \brief Determine whether an electrode has been touched
 *
 * \param electrode Electrode number
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_is_touched(uint8_t electrode, bool *dst,
                              mpr121_sensor_t *sensor){
    uint16_t touched;
    mpr121_touched(&touched, sensor);
    *dst = (bool) ((touched >> electrode) & 1);
}

/*! \brief Read an electrode's filtered data value
 *
 * \param electrode Electrode number
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 * 
 * The data range of the filtered data is 0 to 1024.
 * \sa mpr121_baseline_value
 */
static void mpr121_filtered_data(uint8_t electrode, uint16_t *dst,
                                 mpr121_sensor_t *sensor){
    mpr121_read16(MPR121_ELECTRODE_FILTERED_DATA_REG + (electrode * 2),
                  dst, sensor);
    // Filtered data is 10-bit
    *dst &= 0x3ff;
}

/*! \brief Read an electrode's baseline value
 *
 * \param electrode Electrode number
 * \param dst Pointer to buffer to receive data
 * \param sensor Pointer to the structure that stores the MPR121 info
 *
 * From the MPR112 datasheet:
 * > Along with the 10-bit electrode filtered data output, each channel
 * > also has a 10-bit baseline value. These values are the output of
 * > the internal baseline filter operation tracking the slow-voltage
 * > variation of the background capacitance change. Touch/release 
 * > detection is made based on the comparison between the 10-bit
 * > electrode filtered data and the 10-bit baseline value.
 * 
 * > Although internally the baseline value is 10-bit, users can only
 * > access the 8 MSB of the 10-bit baseline value through the baseline
 * > value registers.
 * 
 * \sa mpr121_filtered_data
 */
static void mpr121_baseline_value(uint8_t electrode, uint16_t *dst,
                                  mpr121_sensor_t *sensor){
    uint8_t baseline;
    mpr121_read(MPR121_BASELINE_VALUE_REG + electrode, &baseline,
                sensor);
    // From the datasheet: Although internally the baseline value is
    // 10-bit, users can only access the 8 MSB of the 10-bit baseline
    // value through the baseline value registers. The read out from the
    // baseline register must be left shift two bits before comparing it
    // with the 10-bit electrode data.
    *dst = baseline << 2;
}

/*! \brief Set the Max Half Delta
 *
 * The Max Half Delta determines the largest magnitude of variation to
 * pass through the third level filter. See application note MPR121
 * Baseline System (AN3891) for details.
 *
 * \param rising Value in the range 1~63
 * \param falling Value in the range 1~63
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_set_max_half_delta(uint8_t rising, uint8_t falling,
        mpr121_sensor_t *sensor) {
    // Read current configuration then enter stop mode
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    // Write MHD values
    mpr121_write(MPR121_MAX_HALF_DELTA_RISING_REG, rising, sensor);
    mpr121_write(MPR121_MAX_HALF_DELTA_FALLING_REG, falling, sensor);
    // Re-enable electrodes
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
}

/*! \brief Set the Noise Half Delta
 *
 * The Noise Half Delta determines the incremental change when
 * non-noise drift is detected. See application note MPR121 Baseline
 * System (AN3891) for details.
 *
 * \param rising Value in the range 1~63
 * \param falling Value in the range 1~63
 * \param touched Value in the range 1~63
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_set_noise_half_delta(uint8_t rising, uint8_t falling,
        uint8_t touched, mpr121_sensor_t *sensor) {
    // Read current configuration then enter stop mode
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    // Write NHD values
    mpr121_write(MPR121_NOISE_HALF_DELTA_RISING_REG, rising, sensor);
    mpr121_write(MPR121_NOISE_HALF_DELTA_FALLING_REG, falling, sensor);
    mpr121_write(MPR121_NOISE_HALF_DELTA_TOUCHED_REG, touched, sensor);
    // Re-enable electrodes
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
}

/*! \brief Set the Noise Count Limit
 *
 * The Noise Count Limit determines the number of samples consecutively
 * greater than the Max Half Delta necessary before it can be
 * determined that it is non-noise. See application note MPR121 Baseline
 * System (AN3891) for details.
 *
 * \param rising Value in the range 0~255
 * \param falling Value in the range 0~255
 * \param touched Value in the range 0~255
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_set_noise_count_limit(uint8_t rising,
        uint8_t falling, uint8_t touched, mpr121_sensor_t *sensor) {
    // Read current configuration then enter stop mode
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    // Write new NCL values
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_RISING_REG, rising, sensor);
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_FALLING_REG, falling, sensor);
    mpr121_write(MPR121_NOISE_COUNT_LIMIT_TOUCHED_REG, touched, sensor);
    // Re-enable electrodes
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
}

/*! \brief Set the Filter Delay Limit
 *
 * The Filter Delay Limit determines the rate of operation of the
 * filter. A larger number makes it operate slower. See application
 * note MPR121 Baseline System (AN3891) for details.
 *
 * \param rising Value in the range 0~255
 * \param falling Value in the range 0~255
 * \param touched Value in the range 0~255
 * \param sensor Pointer to the structure that stores the MPR121 info
 */
static void mpr121_set_filter_delay_limit(uint8_t rising,
        uint8_t falling, uint8_t touched, mpr121_sensor_t *sensor) {
    // Read current configuration then enter stop mode
    uint8_t config;
    mpr121_read(MPR121_ELECTRODE_CONFIG_REG, &config, sensor);
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, 0x00, sensor);
    // Write new FDL values
    mpr121_write(MPR121_FILTER_DELAY_COUNT_RISING_REG, rising, sensor);
    mpr121_write(MPR121_FILTER_DELAY_COUNT_FALLING_REG, falling,
        sensor);
    mpr121_write(MPR121_FILTER_DELAY_COUNT_TOUCHED_REG, touched,
        sensor);
    // Re-enable electrodes
    mpr121_write(MPR121_ELECTRODE_CONFIG_REG, config, sensor);
}

#endif
