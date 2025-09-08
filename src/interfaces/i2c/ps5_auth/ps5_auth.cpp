/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#include "ps5_auth/ps5_auth.h"
#include <stdlib.h>
#include <string.h>
#include "peripheralmanager.h"

#define INIT_DATA_SIZE 3

PS5Auth::PS5Auth() {
    gpio_init(PS5_AUTH_RESET_PIN);
    gpio_set_dir(PS5_AUTH_RESET_PIN, GPIO_OUT);
    gpio_pull_up(PS5_AUTH_RESET_PIN);

    reset_device(100);
}

void PS5Auth::reset_device(uint32_t delay) {
    gpio_put(PS5_AUTH_RESET_PIN, 0);
    sleep_ms(delay);
    gpio_put(PS5_AUTH_RESET_PIN, 1);
}

void PS5Auth::init() {
    uint8_t unk2[] { 0x23, 0x45, 0x67, 0x89, 0xaa, 0xbb, 0xcc, 0xdd };

    reset_read(PS5_AUTH_ADDR_UNK3, 4, unk3);
    reset_read(PS5_AUTH_ADDR_UNK1, 32, unk1);
    write(PS5_AUTH_ADDR_UNK2, 8, unk2);
    write(0x00, 0, nullptr);
}

void PS5Auth::pre_init() {
    //uint8_t buf[4] {};

    //auto i2c = PeripheralManager::getInstance().getI2C(1);

    //i2c->write(0x69, buf, 0);
    //i2c->write(0x69, buf, 0);
    //i2c->write(0x69, buf, 0);
    //i2c->write(0x6b, buf, 0);
    //i2c->write(0x6b, buf, 0);
    //i2c->write(0x6b, buf, 0);
    //i2c->write(m_address, buf, 1);
    //i2c->read(m_address, buf, 4);
    //m_inited = true;

    reset_device(300);
}

void PS5Auth::set_address(uint8_t addr) {
    m_address = addr;
}

void PS5Auth::set_i2c(PeripheralI2C *i2c) {
    m_i2c = i2c;
}

int16_t PS5Auth::reset_read(uint8_t addr, uint8_t size, uint8_t *buf) {
    write(addr, 0, nullptr);
    return read(addr, size, buf);
}

int16_t PS5Auth::read(uint8_t addr, uint8_t size, uint8_t *buf) {
    uint8_t sendbuf[3] = { PS5_AUTH_CMD_READ, addr, size };
    m_i2c->write(m_address, sendbuf, 3);
    return m_i2c->read(m_address, buf, size);
}

int16_t PS5Auth::write(uint8_t addr, uint8_t size, const uint8_t *buf) {
    uint8_t *sendbuf = (uint8_t*)malloc(size + 3);
    sendbuf[0] = PS5_AUTH_CMD_WRITE;
    sendbuf[1] = addr;
    sendbuf[2] = size;
    if (size > 0) {
        memcpy(sendbuf + 3, buf, size);
    }
    return m_i2c->write(m_address, sendbuf, size + 3);
}

int16_t PS5Auth::get_signature(const uint8_t *data, uint8_t *sig) {
    int16_t res;
    uint8_t buf[12];
    write(PS5_AUTH_ADDR_SIGN, 12, data);
    res = m_i2c->read(m_address, buf, 12);
    memcpy(sig, buf + 4, 8);
    return res;
}

int16_t PS5Auth::process_challenge(const uint8_t *challenge, uint8_t *response) {
    write(PS5_AUTH_ADDR_CHALLENGE, 180, challenge);
    return reset_read(PS5_AUTH_ADDR_RESPONSE, 192, response);
}
