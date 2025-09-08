/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#include <iostream>
#include "drivers/ps5/PS5Driver.h"
#include "drivers/shared/driverhelper.h"
#include "peripheralmanager.h"

static constexpr uint8_t macaddr[] =  {0x9, 0xb5, 0xc8, 0x76, 0x4c, 0x3, 0x88, 0x8, 0x25, 0x0, 0x66, 0xb1, 0x3a, 0x0, 0x9e, 0x2c, 0x0, 0x0, 0x0, 0x0};

static constexpr uint8_t calibration[] = {0x5, 0xfe, 0xff, 0x0, 0x0, 0x1, 0x0, 0x9a, 0x22, 0x60, 0xdd, 0xa1, 0x22, 0x5d, 0xdd, 0xac, 0x22, 0x58, 0xdd, 0x1c, 0x2, 0x1c, 0x2, 0x15, 0x1f, 0xea, 0xe0, 0xc7, 0x1e, 0x38, 0xe1, 0xe5, 0x1f, 0x1a, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

static constexpr uint8_t fw_info[] = {0x40, 0x88, 0x97, 0xfe, 0x87, 0xa0, 0xff, 0xff, 0x43, 0x2, 0x80, 0xe, 0x1, 0x0, 0x2d, 0x0, 0x26, 0x3, 0x71, 0x68, 0x0, 0x0, 0x0, 0x0, 0xfa, 0x24, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x20, 0x4a, 0x61, 0x6e, 0x20, 0x32, 0x39, 0x20, 0x32, 0x30, 0x33, 0x34, 0x30, 0x39, 0x3a, 0x31, 0x35, 0x3a, 0x35, 0x39, 0x2, 0x0, 0x4, 0x0, 0x13, 0x5, 0x0, 0x0, 0xa, 0x0, 0xc, 0x1, 0xc0, 0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x88, 0x9, 0x0, 0x0, 0x2a, 0x0, 0x1, 0x0, 0x9, 0x0, 0x2, 0x0, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

static const uint8_t ps5_string_language[] = { 0x09, 0x04 };
static const uint8_t ps5_string_manufacturer[] = "Open Stick Community";
static const uint8_t ps5_string_product[] = "GP2040-CE (PS5)";
static const uint8_t ps5_string_version[] = "1.0";

static const uint8_t *ps5_string_descriptors[] =
{
	ps5_string_language,
	ps5_string_manufacturer,
	ps5_string_product,
	ps5_string_version
};

void PS5Driver::initialize() {
    stdio_init_all();
    // TODO: touchpadData
    // TODO: sensorData

    // TODO: input report init
    m_input_report = {
        .report_id = 0x01,
        .left_stick_x = DUALSENSE_JOYSTICK_MID,
        .left_stick_y = DUALSENSE_JOYSTICK_MID,
        .right_stick_x = DUALSENSE_JOYSTICK_MID,
        .right_stick_y = DUALSENSE_JOYSTICK_MID,
        .dpad = DUALSENSE_HAT_NOTHING,
        .button_west = 0, .button_south = 0, .button_east = 0, .button_north = 0,
        .button_l1 = 0, .button_r1 = 0, .button_l2 = 0, .button_r2 = 0,
        .button_select = 0, .button_start = 0, .button_l3 = 0, .button_r3 = 0, .button_home = 0,
        .button_touchpad = 0, .button_mute = 0,
        .gyro = {}, .accel = {}, .sensor_timestamp = 0,
        .points = {},
        .status = 0,
    };

    class_driver = {
    #if CFG_TUSB_DEBUG >= 2
        .name = "PS5",
    #endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = hidd_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL
    };

    //if (result.address > -1) {
        //m_auth.set_address(result.address);
        //m_auth.set_i2c(PeripheralManager::getInstance().getI2C(result.block));
    //m_auth.pre_init();
    //}

    // TODO: this is kinda stupid, be more straightforward
    //PeripheralI2CScanResult result = PeripheralManager::getInstance().scanForI2CDevice(m_auth.getDeviceAddresses());
    //if (result.address > -1) {
        //m_auth.set_address(result.address);
        //m_auth.set_i2c(PeripheralManager::getInstance().getI2C(result.block));
        //m_auth.init();
    //}
    printf("ps5_native initialized\n");
}

void PS5Driver::initializeAux() {
    // TODO: auth init
    m_auth = new PS5Auth();
    // If authentication driver is set AND auth driver can load (usb enabled, i2c enabled, keys loaded, etc.)
    if ( m_auth != nullptr && m_auth->available() ) {
        m_auth->initialize();
    }
    printf("ps5_native aux initialized\n");
}

// TODO: Most of this function is common
bool PS5Driver::process(Gamepad *gamepad) {
    bool reportSent = false;

    switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD)
    {
        case GAMEPAD_MASK_UP:
            m_input_report.dpad = DUALSENSE_HAT_UP;
            break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:
            m_input_report.dpad = DUALSENSE_HAT_UPRIGHT;
            break;
        case GAMEPAD_MASK_RIGHT:
            m_input_report.dpad = DUALSENSE_HAT_RIGHT;
            break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT:
            m_input_report.dpad = DUALSENSE_HAT_DOWNRIGHT;
            break;
        case GAMEPAD_MASK_DOWN:
            m_input_report.dpad = DUALSENSE_HAT_DOWN;
            break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:
            m_input_report.dpad = DUALSENSE_HAT_DOWNLEFT;
            break;
        case GAMEPAD_MASK_LEFT:
            m_input_report.dpad = DUALSENSE_HAT_LEFT;
            break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:
            m_input_report.dpad = DUALSENSE_HAT_UPLEFT;
            break;
        default:
            m_input_report.dpad = DUALSENSE_HAT_NOTHING;
            break;
    }

    m_input_report.button_south    = gamepad->pressedB1();
    m_input_report.button_east     = gamepad->pressedB2();
    m_input_report.button_west     = gamepad->pressedB3();
    m_input_report.button_north    = gamepad->pressedB4();
    m_input_report.button_l1       = gamepad->pressedL1();
    m_input_report.button_r1       = gamepad->pressedR1();
    m_input_report.button_l2       = gamepad->pressedL2();
    m_input_report.button_r2       = gamepad->pressedR2();
    m_input_report.button_select   = gamepad->pressedS1();
    m_input_report.button_start    = gamepad->pressedS2();
    m_input_report.button_l3       = gamepad->pressedL3();
    m_input_report.button_r3       = gamepad->pressedR3();
    m_input_report.button_home     = gamepad->pressedA1();
    m_input_report.button_touchpad = gamepad->pressedA2();
    m_input_report.button_touchpad = gamepad->pressedA2();
    m_input_report.button_mute     = gamepad->pressedA3();

    m_input_report.left_stick_x = static_cast<uint8_t>(gamepad->state.lx >> 8);
    m_input_report.left_stick_y = static_cast<uint8_t>(gamepad->state.ly >> 8);
    m_input_report.right_stick_x = static_cast<uint8_t>(gamepad->state.rx >> 8);
    m_input_report.right_stick_y = static_cast<uint8_t>(gamepad->state.ry >> 8);

    if (gamepad->hasAnalogTriggers)
    {
        m_input_report.left_trigger = gamepad->state.lt;
        m_input_report.right_trigger = gamepad->state.rt;
    } else {
        m_input_report.left_trigger = gamepad->pressedL2() ? 0xFF : 0;
        m_input_report.right_trigger = gamepad->pressedR2() ? 0xFF : 0;
    }

    // TODO: touchpad/senseors

    // TODO: form and send report

    // TODO: look into tinyusb
    // Wake up TinyUSB device
    if (tud_suspended())
        tud_remote_wakeup();

    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint16_t report_size = sizeof(m_input_report);

    // TODO: last report stuff refactor?
    if (memcmp(m_last_report, &m_input_report, report_size) != 0) {
        // HID ready + report sent, copy previous report
        if (tud_hid_ready() && tud_hid_report(0, &m_input_report, report_size) == true ) {
            memcpy(m_last_report, &m_input_report, report_size);
            reportSent = true;
        }
        // keep track of our last successful report, for keepalive purposes
        m_last_report_timer = now;
    } else {
        // some games apparently can miss reports, or they rely on official behavior of getting frequent
        // updates. we normally only send a report when the value changes; if we increment the counters
        // every time we generate the report (every GP2040::run loop), we apparently overburden
        // TinyUSB and introduce roughly 1ms of latency. but we want to loop often and report on every
        // true update in order to achieve our tight <1ms report timing when we *do* have a different
        // report to send.
        if ((now - m_last_report_timer) > PS5_KEEPALIVE_TIMER) {
            m_input_report.sensor_timestamp = now;		 		// axis counter is 16 bits
            // the *next* process() will be a forced report (or real user input)
        }
    }

    return reportSent;
}

void PS5Driver::processAux() {
    // TODO: auth process
}

uint16_t PS5Driver::get_report(uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    printf("ps5_native %s: 0x%02x [0x%02x]\n", __func__, report_id, report_type);
    if (report_type != HID_REPORT_TYPE_FEATURE) {
        memcpy(buffer, &m_input_report, sizeof(m_input_report));
        return sizeof(m_input_report);
    }
    // TODO: process get report

    switch(report_id) {
        case DS_FEATURE_REPORT_CALIBRATION:
            memcpy(buffer, calibration, PS5_CALIBRATION_SIZE);
            return PS5_CALIBRATION_SIZE;
        case DS_FEATURE_REPORT_MACADDR:
            memcpy(buffer, macaddr, PS5_MACADDR_SIZE);
            return PS5_MACADDR_SIZE;
        case DS_FEATURE_REPORT_FW_INFO:
            memcpy(buffer, fw_info, PS5_FW_INFO_SIZE);
            return PS5_FW_INFO_SIZE;
        default:
            break;
    };

    return -1;
};

void PS5Driver::set_report(uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    printf("ps5_native %s: 0x%02x [0x%02x]\n", __func__, report_id, report_type);
    // TODO: process set report
}

// Xbox specific
bool PS5Driver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                       tusb_control_request_t const *request) {
    return false;
}

const uint16_t *PS5Driver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    const char *value = (const char *)ps5_string_descriptors[index];
    return getStringDescriptor(value, index);
}

const uint8_t *PS5Driver::get_descriptor_device_cb() {
    return ps5_device_descriptor;
}

const uint8_t *PS5Driver::get_hid_descriptor_report_cb(uint8_t itf) {
    return ps5_report_descriptor;
}

const uint8_t *PS5Driver::get_descriptor_configuration_cb(uint8_t index) {
    return ps5_configuration_descriptor;
}

const uint8_t *PS5Driver::get_descriptor_device_qualifier_cb() {
    return nullptr;
}

uint16_t PS5Driver::GetJoystickMidValue() {
    return DUALSENSE_JOYSTICK_MID << 8;
}

USBListener *PS5Driver::get_usb_auth_listener() {
    // TODO: return auth driver listener
    return nullptr;
}
