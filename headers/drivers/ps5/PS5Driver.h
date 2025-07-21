/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _PS5_DRIVER_H_
#define _PS5_DRIVER_H_

#include "gpdriver.h"
#include "drivers/ps5/PS5Descriptors.h"
#include "ps5_auth/ps5_auth.h"

typedef enum {
    PS5_GET_CALIBRATION = 0x05,
    PS5_GET_MACADDR = 0x09,
    PS5_GET_FW_INFO = 0x20,
} PS5ReportID;

#define PS5_CALIBRATION_SIZE 41
#define PS5_MACADDR_SIZE 20
#define PS5_FW_INFO_SIZE 64

// TODO: common
#define PS5_KEEPALIVE_TIMER 5

class PS5Driver : public GPDriver {
public:
    void initialize() override;
    void initializeAux() override;
    bool process(Gamepad *gamepad) override;
    void processAux() override;
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type,
                        uint8_t *buffer, uint16_t reqlen) override;
    void set_report(uint8_t report_id, hid_report_type_t report_type,
                    uint8_t const *buffer, uint16_t bufsize) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request) override;
    const uint16_t *get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t *get_descriptor_device_cb() override;
    const uint8_t *get_hid_descriptor_report_cb(uint8_t itf) override;
    const uint8_t *get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t *get_descriptor_device_qualifier_cb() override;
    uint16_t GetJoystickMidValue() override;
    USBListener *get_usb_auth_listener() override;

private:
    PS5Auth m_auth;
    uint8_t m_last_report[CFG_TUD_ENDPOINT0_SIZE] {};
    uint32_t m_last_report_timer;
    ps5_input_report m_input_report {};
};

#endif // _PS5_DRIVER_H_
