/*
 * MP121 Captive Touch Sensor
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include "hardware/i2c.h"

#include "mpr121.h"
#include "board_defs.h"

#define IO_TIMEOUT_US 1000

#define TOUCH_THRESHOLD_BASE 22
#define RELEASE_THRESHOLD_BASE 15

#define MPR121_TOUCH_STATUS_REG 0x00
#define MPR121_OUT_OF_RANGE_STATUS_0_REG 0x02
#define MPR121_OUT_OF_RANGE_STATUS_1_REG 0x03
#define MPR121_ELECTRODE_FILTERED_DATA_REG 0x04
#define MPR121_BASELINE_VALUE_REG 0x1E

#define MPR121_MAX_HALF_DELTA_RISING_REG 0x2B
#define MPR121_NOISE_HALF_DELTA_RISING_REG 0x2C
#define MPR121_NOISE_COUNT_LIMIT_RISING_REG 0x2D
#define MPR121_FILTER_DELAY_COUNT_RISING_REG 0x2E
#define MPR121_MAX_HALF_DELTA_FALLING_REG 0x2F
#define MPR121_NOISE_HALF_DELTA_FALLING_REG 0x30
#define MPR121_NOISE_COUNT_LIMIT_FALLING_REG 0x31
#define MPR121_FILTER_DELAY_COUNT_FALLING_REG 0x32
#define MPR121_NOISE_HALF_DELTA_TOUCHED_REG 0x33
#define MPR121_NOISE_COUNT_LIMIT_TOUCHED_REG 0x34
#define MPR121_FILTER_DELAY_COUNT_TOUCHED_REG 0x35

#define MPR121_TOUCH_THRESHOLD_REG 0x41
#define MPR121_RELEASE_THRESHOLD_REG 0x42

#define MPR121_DEBOUNCE_REG 0x5B
#define MPR121_AFE_CONFIG_REG 0x5C
#define MPR121_FILTER_CONFIG_REG 0x5D
#define MPR121_ELECTRODE_CONFIG_REG 0x5E
#define MPR121_ELECTRODE_CURRENT_REG 0x5F
#define MPR121_ELECTRODE_CHARGE_TIME_REG 0x6C
#define MPR121_GPIO_CTRL_0_REG 0x73
#define MPR121_GPIO_CTRL_1_REG 0x74
#define MPR121_GPIO_DATA_REG 0x75
#define MPR121_GPIO_DIRECTION_REG 0x76
#define MPR121_GPIO_ENABLE_REG 0x77
#define MPR121_GPIO_DATA_SET_REG 0x78
#define MPR121_GPIO_DATA_CLEAR_REG 0x79
#define MPR121_GPIO_DATA_TOGGLE_REG 0x7A
#define MPR121_AUTOCONFIG_CONTROL_0_REG 0x7B
#define MPR121_AUTOCONFIG_CONTROL_1_REG 0x7C
#define MPR121_AUTOCONFIG_USL_REG 0x7D
#define MPR121_AUTOCONFIG_LSL_REG 0x7E
#define MPR121_AUTOCONFIG_TARGET_REG 0x7F
#define MPR121_SOFT_RESET_REG 0x80

static void write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[] = {reg, val};
    i2c_write_blocking_until(I2C_PORT, addr, buf, 2, false,
                             time_us_64() + IO_TIMEOUT_US);
}

static uint8_t read_reg(uint8_t addr, uint8_t reg)
{
    uint8_t value;
    i2c_write_blocking_until(I2C_PORT, addr, &reg, 1, true,
                             time_us_64() + IO_TIMEOUT_US);
    i2c_read_blocking_until(I2C_PORT, addr, &value, 1, false,
                            time_us_64() + IO_TIMEOUT_US);
    return value;
}

void mpr121_init(uint8_t i2c_addr)
{
    write_reg(i2c_addr, 0x80, 0x63); // Soft reset MPR121 if not reset correctly 

    //touch pad baseline filter 
    //rising: baseline quick rising 
    write_reg(i2c_addr, 0x2B, 1); // Max half delta Rising 
    write_reg(i2c_addr, 0x2C, 1); // Noise half delta Rising 
    write_reg(i2c_addr, 0x2D, 1); // Noise count limit Rising 
    write_reg(i2c_addr, 0x2E, 1); // Delay limit Rising

    //falling: baseline slow falling 
    write_reg(i2c_addr, 0x2F, 1); // Max half delta Falling 
    write_reg(i2c_addr, 0x30, 1); // Noise half delta Falling 
    write_reg(i2c_addr, 0x31, 6); // Noise count limit Falling 
    write_reg(i2c_addr, 0x32, 12); // Delay limit Falling

    //touched: baseline very slow falling
    write_reg(i2c_addr, 0x33, 1); // Noise half delta Touched 
    write_reg(i2c_addr, 0x34, 8); // Noise count Touched 
    write_reg(i2c_addr, 0x35, 30); // Delay limit Touched 

    //Touch pad threshold 
    for (int i = 0; i < 12; i++) {
        write_reg(i2c_addr, 0x41 + i * 2, TOUCH_THRESHOLD_BASE);
        write_reg(i2c_addr, 0x42 + i * 2, RELEASE_THRESHOLD_BASE);
    }

    //touch and release debounce 
    write_reg(i2c_addr, 0x5B, 0x00);

    //AFE and filter configuration 
    write_reg(i2c_addr, 0x5C, 0b00010000); // AFES=6 samples, same as AFES in 0x7B, Global CDC=16uA 
    write_reg(i2c_addr, 0x5D, 0b00101000); // CT=0.5us, TDS=4samples, TDI=16ms 
    write_reg(i2c_addr, 0x5E, 0x80); // Set baseline calibration enabled, baseline loading 5MSB 

    //Auto Configuration 
    write_reg(i2c_addr, 0x7B, 0b00001011); // AFES=6 samples, same as AFES in 0x5C 
    // retry=2b00, no retry, 
    // BVA=2b10, load 5MSB after AC, 
    // ARE/ACE=2b11, auto configuration enabled 
    //write_reg(i2c_addr, 0x7C,0x80); // Skip charge time search, use setting in 0x5D, 
    // OOR, AR, AC IE disabled 
    // Not used. Possible Proximity CDC shall over 63uA 
    // if only use 0.5uS CDT, the TGL for proximity cannot meet 
    // Possible if manually set Register0x72=0x03 
    // (Auto configure result) alone. 

    // I want to max out sensitivity, I don't care linearity
    const uint8_t usl = (3.3 - 0.1) / 3.3 * 256;
    write_reg(i2c_addr, 0x7D, usl),  
    write_reg(i2c_addr, 0x7E, usl * 0.65),
    write_reg(i2c_addr, 0x7F, usl * 0.9);

    write_reg(i2c_addr, 0x5E, 0x8C); // Run 12 touch, load 5MSB to baseline 
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

static void mpr121_read_many(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n)
{
    i2c_write_blocking_until(I2C_PORT, addr, &reg, 1, true,
                             time_us_64() + IO_TIMEOUT_US);
    i2c_read_blocking_until(I2C_PORT, addr, buf, n, false,
                             time_us_64() + IO_TIMEOUT_US * n / 2);
}

static void mpr121_read_many16(uint8_t addr, uint8_t reg, uint16_t *buf, size_t n)
{
    uint8_t vals[n * 2];
    mpr121_read_many(addr, reg, vals, n * 2);
    for (int i = 0; i < n; i++) {
        buf[i] = (vals[i * 2 + 1] << 8) | vals[i * 2];
    }
}

uint16_t mpr121_touched(uint8_t addr)
{
    uint16_t touched;
    mpr121_read_many16(addr, MPR121_TOUCH_STATUS_REG, &touched, 2);
    return touched;
}

void mpr121_raw(uint8_t addr, uint16_t *raw, int num)
{
    mpr121_read_many16(addr, MPR121_ELECTRODE_FILTERED_DATA_REG, raw, num);
}

static uint8_t mpr121_stop(uint8_t addr)
{
    uint8_t ecr = read_reg(addr, MPR121_ELECTRODE_CONFIG_REG);
    write_reg(addr, MPR121_ELECTRODE_CONFIG_REG, ecr & 0xC0);
    return ecr;
}

static uint8_t mpr121_resume(uint8_t addr, uint8_t ecr)
{
    write_reg(addr, MPR121_ELECTRODE_CONFIG_REG, ecr);
}

void mpr121_filter(uint8_t addr, uint8_t ffi, uint8_t sfi, uint8_t esi)
{
    uint8_t ecr = mpr121_stop(addr);

    uint8_t afe = read_reg(addr, MPR121_AFE_CONFIG_REG);
    write_reg(addr, MPR121_AFE_CONFIG_REG, (afe & 0x3f) | ffi << 6);
    uint8_t acc = read_reg(addr, MPR121_AUTOCONFIG_CONTROL_0_REG);
    write_reg(addr, MPR121_AUTOCONFIG_CONTROL_0_REG, (acc & 0x3f) | ffi << 6);
    uint8_t fcr = read_reg(addr, MPR121_FILTER_CONFIG_REG);
    write_reg(addr, MPR121_FILTER_CONFIG_REG,
              (fcr & 0xe0) | ((sfi & 3) << 3) | esi);

    mpr121_resume(addr, ecr);
}

void mpr121_sense(uint8_t addr, int8_t sense, int8_t *sense_keys)
{
    uint8_t ecr = mpr121_stop(addr);
    for (int i = 0; i < 12; i++) {
        int8_t delta = sense + sense_keys[i];
        write_reg(addr, MPR121_TOUCH_THRESHOLD_REG + i * 2,
                        TOUCH_THRESHOLD_BASE - delta);
        write_reg(addr, MPR121_RELEASE_THRESHOLD_REG + i * 2,
                        RELEASE_THRESHOLD_BASE - delta / 2);
    }
    mpr121_resume(addr, ecr);
}

void mpr121_debounce(uint8_t addr, uint8_t touch, uint8_t release)
{
    uint8_t ecr = mpr121_stop(addr);
    write_reg(addr, 0x5B, (release & 0x07) << 4 | (touch & 0x07));
    mpr121_resume(addr, ecr);
}
