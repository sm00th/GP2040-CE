#include "host/usbh.h"
#include "class/hid/hid.h"
#include "class/hid/hid_host.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include "peripheralmanager.h"
#include "usbhostmanager.h"

#include <iostream>
#define DEBUG_PREFIX "ps5auth_usb: "

PS5AuthUSBListener::~PS5AuthUSBListener() {
    for (auto cache_item: m_cache) {
        if (cache_item.buf == nullptr) {
            free(cache_item.buf);
            cache_item.buf = nullptr;
        }
    }
}

void PS5AuthUSBListener::reset() {
    m_ps_dev_addr = 0xff;
    m_ps_instance = 0xff;
    m_state = PS5_AUTH_LISTENER_STATE_NOT_READY;
    m_pending_report = 0xff;

    for (auto cache_item: m_cache) {
        if (cache_item.buf == nullptr) {
            cache_item.buf = static_cast<uint8_t*>(malloc(cache_item.report_len));
        }
    }
}

void PS5AuthUSBListener::setup() {
    printf(DEBUG_PREFIX "%s\n", __func__);
    reset();
}

uint16_t PS5AuthUSBListener::get_cached_report(uint8_t report_id, void* report, uint16_t len) {
    if (m_state != PS5_AUTH_LISTENER_STATE_READY) {
        return 0;
    }

    for (auto cache_item: m_cache) {
        if (cache_item.report_id != report_id) {
            continue;
        }

        uint16_t cpylen = std::min(len, cache_item.report_len);
        memcpy(report, cache_item.buf, cpylen);
        return cpylen;
    }

    // Not found, let's STALL and hope it will re-inquire.
    PS5AuthUSBListenerCache cache_item {report_id, len, (uint8_t*)malloc(len)};
    m_cache.push_back(cache_item);
    host_get_report(report_id, cache_item.buf, len);
    return 0;
}

bool PS5AuthUSBListener::host_get_report(uint8_t report_id, void *report_buf,
                                         uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, report_id);
    m_pending_report = report_id;
    return tuh_hid_get_report(m_ps_dev_addr, m_ps_instance, report_id,
                              HID_REPORT_TYPE_FEATURE, report_buf, len);
}

bool PS5AuthUSBListener::host_set_report(uint8_t report_id, const void *report_buf,
                                         uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, report_id);
    m_pending_report = report_id;
    return tuh_hid_set_report(m_ps_dev_addr, m_ps_instance, report_id,
                              HID_REPORT_TYPE_FEATURE, (void*)report_buf, len);
}

void PS5AuthUSBListener::init_cache_item() {
    if (m_init_item == m_cache.end()) {
        m_state = PS5_AUTH_LISTENER_STATE_READY;
        return;
    }

    printf(DEBUG_PREFIX "%s: report_id=0x%02x\n", __func__, m_init_item->report_id);
    host_get_report(m_init_item->report_id, m_init_item->buf,
                    m_init_item->report_len);
}

void PS5AuthUSBListener::mount(uint8_t dev_addr, uint8_t instance, uint8_t
                               const *desc_report, uint16_t desc_len) {
    printf(DEBUG_PREFIX "%s: dev_addr=0x%02x, instance=0x%02x\n", __func__,
           dev_addr, instance);

    // Only a PS4 interface has vendor IDs F0, F1, F2, and F3
    tuh_hid_report_info_t report_info[4];
    uint8_t report_count = tuh_hid_parse_report_descriptor(report_info, 4,
                                                           desc_report, desc_len);
    printf(DEBUG_PREFIX "%s: desc_len=%d, report_count=%d\n", __func__,
           desc_len, report_count);

    bool is_ps5_dongle = false;
    for(uint8_t i = 0; i < report_count; i++) {
        if (report_info[i].usage_page == 0x01 &&
                (report_info[i].report_id == 0xf5) ) {
            is_ps5_dongle = true;
            break;
        }
    }
    if (is_ps5_dongle == false) {
        printf(DEBUG_PREFIX "%s: bad dongle\n", __func__);
        return;
    }

    m_ps_dev_addr = dev_addr;
    m_ps_instance = instance;

    m_state = PS5_AUTH_LISTENER_STATE_INIT;
    m_init_item = m_cache.begin();
    init_cache_item();
}

void PS5AuthUSBListener::unmount(uint8_t dev_addr) {
    printf(DEBUG_PREFIX "%s: 0x%02x\n", __func__, dev_addr);
    reset();
}

void PS5AuthUSBListener::set_report_complete(uint8_t dev_addr, uint8_t instance,
                                             uint8_t report_id, uint8_t report_type,
                                             uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x; 0x%02x; 0x%02x; 0x%02x; 0x%02x\n", __func__,
           dev_addr, instance, report_id, report_type, len);

    if (report_id != m_pending_report || dev_addr != m_ps_dev_addr ||
            instance != m_ps_instance) {
        return;
    }
    m_pending_report = 0xff;
}

void PS5AuthUSBListener::get_report_complete(uint8_t dev_addr, uint8_t instance,
                                             uint8_t report_id, uint8_t report_type,
                                             uint16_t len) {
    printf(DEBUG_PREFIX "%s: 0x%02x; 0x%02x; 0x%02x; 0x%02x; 0x%02x\n", __func__,
           dev_addr, instance, report_id, report_type, len);

    if (report_id != m_pending_report || dev_addr != m_ps_dev_addr ||
            instance != m_ps_instance) {
        return;
    }
    m_pending_report = 0xff;

    if (m_state == PS5_AUTH_LISTENER_STATE_INIT) {
        m_init_item++;
        init_cache_item();
    }
}
