/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _PS5_DESCRIPTORS_H_
#define _PS5_DESCRIPTORS_H_

#define LSB(n) (n & 255)
#define MSB(n) ((n >> 8) & 255)

#define DUALSENSE_VENDOR_ID 0x054C
#define DUALSENSE_PRODUCT_ID 0x0CE6

#define DUALSENSE_ENDPOINT0_SIZE 64

#define DUALSENSE_GAMEPAD_SIZE 64
// TODO this is originally 3
#define DUALSENSE_GAMEPAD_INTERFACE 0
#define DUALSENSE_GAMEPAD_ENDPOINT 4

#define DUALSENSE_JOYSTICK_MIN 0x00
#define DUALSENSE_JOYSTICK_MID 0x80
#define DUALSENSE_JOYSTICK_MAX 0xFF

// TODO: these are common between last 2 gens, move to ps-common.h?

// HAT report (4 bits)
#define DUALSENSE_HAT_UP        0x00
#define DUALSENSE_HAT_UPRIGHT   0x01
#define DUALSENSE_HAT_RIGHT     0x02
#define DUALSENSE_HAT_DOWNRIGHT 0x03
#define DUALSENSE_HAT_DOWN      0x04
#define DUALSENSE_HAT_DOWNLEFT  0x05
#define DUALSENSE_HAT_LEFT      0x06
#define DUALSENSE_HAT_UPLEFT    0x07
#define DUALSENSE_HAT_NOTHING   0x0F

// TODO: rename
#define DS_FEATURE_REPORT_CALIBRATION		    0x05
#define DS_FEATURE_REPORT_CALIBRATION_SIZE	    41
#define DS_FEATURE_REPORT_MACADDR		        0x09
#define DS_FEATURE_REPORT_MACADDR_SIZE	        20
#define DS_FEATURE_REPORT_FW_INFO		        0x20
#define DS_FEATURE_REPORT_FW_INFO_SIZE	        64

typedef struct __attribute__((packed)) {
    uint8_t report_id;

    uint8_t valid_flag0;
    uint8_t valid_flag1;

    /* For DualShock 4 compatibility mode. */
    uint8_t motor_right;
    uint8_t motor_left;

    /* Audio controls */
    uint8_t reserved[4];
    uint8_t mute_button_led;

    uint8_t power_save_control;
    uint8_t reserved2[28];

    /* LEDs and lightbar */
    uint8_t valid_flag2;
    uint8_t reserved3[2];
    uint8_t lightbar_setup;
    uint8_t led_brightness;
    uint8_t player_leds;
    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;

    uint8_t padding[15];
} ps5_feature_report;

struct dualsense_touch_point {
    uint8_t contact;
    uint8_t x_lo;
    uint8_t x_hi:4, y_lo:4;
    uint8_t y_hi;
} __packed;

typedef struct __attribute__((packed)) {
    uint8_t report_id;

    uint8_t left_stick_x, left_stick_y;
    uint8_t right_stick_x, right_stick_y;
    uint8_t left_trigger, right_trigger;
    uint8_t seq_number;

    //uint8_t buttons[4];
    uint32_t dpad : 4;
    uint32_t button_west : 1;
    uint32_t button_south : 1;
    uint32_t button_east : 1;
    uint32_t button_north : 1;
    uint32_t button_l1 : 1;
    uint32_t button_r1 : 1;
    uint32_t button_l2 : 1;
    uint32_t button_r2 : 1;
    uint32_t button_select : 1;
    uint32_t button_start : 1;
    uint32_t button_l3 : 1;
    uint32_t button_r3 : 1;
    uint32_t button_home : 1;
    uint32_t button_touchpad : 1;
    uint32_t button_mute : 1;
    uint32_t padding: 13;

    uint8_t reserved[4];

    /* Motion sensors */
    uint16_t gyro[3]; /* x, y, z */
    uint16_t accel[3]; /* x, y, z */
    uint32_t sensor_timestamp;
    uint8_t reserved2;

    /* Touchpad */
    struct dualsense_touch_point points[2];

    uint8_t reserved3[12];
    uint8_t status;
    uint8_t reserved4[10];

} ps5_input_report;

static const uint8_t ps5_device_descriptor[] =
{
    18,                               // bLength
    1,                                // bDescriptorType
    0x00, 0x02,                       // bcdUSB
    0,                                // bDeviceClass
    0,                                // bDeviceSubClass
    0,                                // bDeviceProtocol
    DUALSENSE_ENDPOINT0_SIZE,         // bMaxPacketSize0
    LSB(DUALSENSE_VENDOR_ID), MSB(DUALSENSE_VENDOR_ID),   // idVendor
    LSB(DUALSENSE_PRODUCT_ID), MSB(DUALSENSE_PRODUCT_ID), // idProduct
    0x00, 0x01,                       // bcdDevice
    1,                                // iManufacturer
    2,                                // iProduct
    0,                                // iSerialNumber
    1                                 // bNumConfigurations
};

static const uint8_t ps5_report_descriptor[] =
{
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x06,        //   Report Count (6)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0F,        //   Usage Maximum (0x0F)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0F,        //   Report Count (15)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x0D,        //   Report Count (13)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x22,        //   Usage (0x22)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x34,        //   Report Count (52)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x23,        //   Usage (0x23)
    0x95, 0x2F,        //   Report Count (47)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x05,        //   Report ID (5)
    0x09, 0x33,        //   Usage (0x33)
    0x95, 0x28,        //   Report Count (40)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x08,        //   Report ID (8)
    0x09, 0x34,        //   Usage (0x34)
    0x95, 0x2F,        //   Report Count (47)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x09,        //   Report ID (9)
    0x09, 0x24,        //   Usage (0x24)
    0x95, 0x13,        //   Report Count (19)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x0A,        //   Report ID (10)
    0x09, 0x25,        //   Usage (0x25)
    0x95, 0x1A,        //   Report Count (26)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x20,        //   Report ID (32)
    0x09, 0x26,        //   Usage (0x26)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x21,        //   Report ID (33)
    0x09, 0x27,        //   Usage (0x27)
    0x95, 0x04,        //   Report Count (4)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x22,        //   Report ID (34)
    0x09, 0x40,        //   Usage (0x40)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x80,        //   Report ID (-128)
    0x09, 0x28,        //   Usage (0x28)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x81,        //   Report ID (-127)
    0x09, 0x29,        //   Usage (0x29)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x82,        //   Report ID (-126)
    0x09, 0x2A,        //   Usage (0x2A)
    0x95, 0x09,        //   Report Count (9)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x83,        //   Report ID (-125)
    0x09, 0x2B,        //   Usage (0x2B)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x84,        //   Report ID (-124)
    0x09, 0x2C,        //   Usage (0x2C)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x85,        //   Report ID (-123)
    0x09, 0x2D,        //   Usage (0x2D)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA0,        //   Report ID (-96)
    0x09, 0x2E,        //   Usage (0x2E)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xE0,        //   Report ID (-32)
    0x09, 0x2F,        //   Usage (0x2F)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF0,        //   Report ID (-16)
    0x09, 0x30,        //   Usage (0x30)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF1,        //   Report ID (-15)
    0x09, 0x31,        //   Usage (0x31)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF2,        //   Report ID (-14)
    0x09, 0x32,        //   Usage (0x32)
    0x95, 0x0F,        //   Report Count (15)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF4,        //   Report ID (-12)
    0x09, 0x35,        //   Usage (0x35)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF5,        //   Report ID (-11)
    0x09, 0x36,        //   Usage (0x36)
    0x95, 0x03,        //   Report Count (3)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

#define DUALSENSE_CONFIG1_DESC_SIZE (9+9+9+7+7)
static const uint8_t ps5_configuration_descriptor[] =
{
    // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
    9,                                  // bLength;
    2,                                  // bDescriptorType;
    LSB(DUALSENSE_CONFIG1_DESC_SIZE),   // wTotalLength
    MSB(DUALSENSE_CONFIG1_DESC_SIZE),
    1,                                  // bNumInterfaces
    1,                                  // bConfigurationValue
    0,                                  // iConfiguration
    0x80,                               // bmAttributes 0xC0
    50,                                 // bMaxPower 250 real
    // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
    9,                                  // bLength
    4,                                  // bDescriptorType
    DUALSENSE_GAMEPAD_INTERFACE,        // bInterfaceNumber
    0,                                  // bAlternateSetting
    2,                                  // bNumEndpoints
    0x03,                               // bInterfaceClass (0x03 = HID)
    0x00,                               // bInterfaceSubClass (0x00 = No Boot)
    0x00,                               // bInterfaceProtocol (0x00 = No Protocol)
    0,                                  // iInterface
    // HID interface descriptor, HID 1.11 spec, section 6.2.1
    9,                                  // bLength
    0x21,                               // bDescriptorType
    0x11, 0x01,                         // bcdHID
    0,                                  // bCountryCode
    1,                                  // bNumDescriptors
    0x22,                               // bDescriptorType
    LSB(sizeof(ps5_report_descriptor)), // wDescriptorLength (273)
    MSB(sizeof(ps5_report_descriptor)),
    // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
    7,                                  // bLength
    5,                                  // bDescriptorType
    DUALSENSE_GAMEPAD_ENDPOINT | 0x80,  // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    DUALSENSE_GAMEPAD_SIZE, 0,          // wMaxPacketSize
    6,                                  // bInterval (1 ms)
    0x07, 0x05, 0x03, 0x03, 0x40, 0x00, 0x06
};

#endif // _PS5_DESCRIPTORS_H_
