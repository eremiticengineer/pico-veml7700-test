#include <stdio.h>

#include "VEML7700.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <cmath>

#define VEML7700_ALS_CONF    0x00
#define VEML7700_ALS_DATA    0x04

VEML7700::VEML7700(i2c_inst_t *i2c_inst, uint8_t address)
    : i2c(i2c_inst), i2c_address(address) {}

bool VEML7700::begin() {
    uint16_t config = 0x0000;

    if (!writeRegister(VEML7700_ALS_CONF, config)) {
        printf("VEML7700 config write failed\n");
        return false;
    }

    uint16_t readback;

    if (!readRegister(VEML7700_ALS_CONF, readback)) {
        printf("VEML7700 config read failed\n");
        return false;
    }

    printf("VEML7700 config = 0x%04X\n", readback);

    return true;
}

// void VEML7700::configure() {
//     // Set ALS gain = 1x, integration time = 100 ms, ALS enable
//     uint16_t config = 0x0000;
//     config |= (0x00 << 11); // Gain 1x
//     config |= (0x02 << 6);  // IT 100 ms
//     config |= (0x00 << 1);  // ALS power on

//     writeRegister(VEML7700_ALS_CONF, config);
// }

void VEML7700::configure() {
    // Gain = 1x      (00 << 11)
    // IT = 100ms     (110 << 6)
    // Persistence = 1 (00 << 4)
    // Power on       (0 << 1)
    // ALS enable     (0 << 0)

    uint16_t config = 0x0000;
    config |= (0x0 << 11); // gain: 00 = 1x
    config |= (0x6 << 6);  // IT: 110 = 100ms
    config |= (0x0 << 4);  // persistence: 00 = 1 sample
    config |= (0x0 << 1);  // power mode: 0 = on
    config |= (0x0 << 0);  // ALS enable: 0 = ALS on

    writeRegister(VEML7700_ALS_CONF, config);
}

bool VEML7700::readLux(float &lux) {
    uint16_t raw;
    if (!readRegister(VEML7700_ALS_DATA, raw)) {
        return false;
    }

    // According to datasheet, lux = raw * (0.0576) for gain=1x, IT=100ms
    lux = raw * 0.0576f;
    return true;
}

bool VEML7700::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = {
        reg,
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>(value >> 8)
    };

    int result = i2c_write_blocking(
        i2c,
        i2c_address,
        buf,
        3,
        false
    );

    printf("VEML write: %d\n", result);

    return result == 3;
}

bool VEML7700::readRegister(uint8_t reg, uint16_t &value) {
    int result = i2c_write_blocking(
        i2c,
        i2c_address,
        &reg,
        1,
        true
    );

    printf("VEML register select: %d\n", result);

    if (result != 1) {
        return false;
    }

    uint8_t buf[2];

    result = i2c_read_blocking(
        i2c,
        i2c_address,
        buf,
        2,
        false
    );

    printf("VEML read: %d\n", result);

    if (result != 2) {
        return false;
    }

    value = buf[0] | (static_cast<uint16_t>(buf[1]) << 8);

    return true;
}
