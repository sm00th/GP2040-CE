#include "host/usbh.h"
#include "class/hid/hid.h"
#include "class/hid/hid_host.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include "peripheralmanager.h"
#include "usbhostmanager.h"

#include <iostream>
#define DEBUG_PREFIX "ps5auth_usb: "

void PS5AuthUSBListener::setup() {
    ps_dev_addr = 0xFF;
    ps_instance = 0xFF;
    awaiting_cb = false;
    printf(DEBUG_PREFIX "%s\n", __func__);
}

bool PS5AuthUSBListener::host_get_report(uint8_t report_id, void* report, uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, report_id);
    awaiting_cb = true;
    return tuh_hid_get_report(ps_dev_addr, ps_instance, report_id, HID_REPORT_TYPE_FEATURE, report, len);
}

bool PS5AuthUSBListener::host_set_report(uint8_t report_id, void* report, uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, report_id);
    awaiting_cb = true;
    return tuh_hid_set_report(ps_dev_addr, ps_instance, report_id, HID_REPORT_TYPE_FEATURE, report, len);
}

void PS5AuthUSBListener::mount(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    printf(DEBUG_PREFIX "%s: 0x%02x, 0x%02x\n", __func__, dev_addr, instance);

    // Only a PS4 interface has vendor IDs F0, F1, F2, and F3
    tuh_hid_report_info_t report_info[4];
    uint8_t report_count = tuh_hid_parse_report_descriptor(report_info, 4, desc_report, desc_len);
    bool is_ps5_dongle = false;
    for(uint8_t i = 0; i < report_count; i++) {
        if (report_info[i].usage_page == 0xFFF0 && 
                (report_info[i].report_id == 0xF3) ) {
            is_ps5_dongle = true;
            break;
        }
    }
    if (is_ps5_dongle == false) {
        printf(DEBUG_PREFIX "%s: bad dongle\n", __func__);
        return;
    }

    ps_dev_addr = dev_addr;
    ps_instance = instance;
}

void PS5AuthUSBListener::unmount(uint8_t dev_addr) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, dev_addr);

    ps_dev_addr = 0xFF;
    ps_instance = 0xFF;
}

void PS5AuthUSBListener::set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x; 0x%02x; 0x%02x; 0x%02x; 0x%02x\n", __func__, dev_addr, instance, report_id, report_type, len);
    if ((dev_addr != ps_dev_addr) || (instance != ps_instance)) {
        return;
    }

    awaiting_cb = false;
}

void PS5AuthUSBListener::get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x; 0x%02x; 0x%02x; 0x%02x; 0x%02x\n", __func__, dev_addr, instance, report_id, report_type, len);
    if ((dev_addr != ps_dev_addr) || (instance != ps_instance)) {
        return;
    }

    awaiting_cb = false;
}
