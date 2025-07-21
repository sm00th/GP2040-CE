/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _PS5_AUTH_H_
#define _PS5_AUTH_H_

#include "i2cdevicebase.h"

enum PS5_AUTH_CMD {
    PS5_AUTH_CMD_WRITE = 0x00,
    PS5_AUTH_CMD_READ = 0x83,
};

enum PS5_AUTH_ADDR {
    PS5_AUTH_ADDR_UNK1 = 0x60,
    PS5_AUTH_ADDR_UNK2 = 0x70,
    PS5_AUTH_ADDR_UNK3 = 0x78,
    PS5_AUTH_ADDR_CHALLENGE = 0x88,
    PS5_AUTH_ADDR_SIGN = 0xa0,
    PS5_AUTH_ADDR_RESPONSE = 0xa8,
};

class PS5Auth : public I2CDeviceBase {
    public:
        PS5Auth() {};
        PS5Auth(PeripheralI2C *i2c, uint8_t addr);
        std::vector<uint8_t> getDeviceAddresses() const override {
            return {0x1a};
        }

        void init();
        void set_address(uint8_t addr);
        void set_i2c(PeripheralI2C *i2c);

        int16_t get_signature(const uint8_t *data, uint8_t *sig);
        int16_t process_challenge(const uint8_t *challenge, uint8_t *response);

    protected:
        int16_t reset_read(uint8_t addr, uint8_t size, uint8_t *buf);
        int16_t read(uint8_t addr, uint8_t size, uint8_t *buf);
        int16_t write(uint8_t addr, uint8_t size, const uint8_t *data);

        PeripheralI2C *m_i2c;
        uint8_t m_address;
};

#endif /* _PS5_AUTH_H_ */
