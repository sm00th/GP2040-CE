#ifndef _PS5AUTHUSBLISTENER_H_
#define _PS5AUTHUSBLISTENER_H_

#include "usblistener.h"
#include "PS5Auth.h"
#include "PS5Descriptors.h"
#include <vector>

enum PS5AuthUSBListenerState {
    PS5_AUTH_LISTENER_STATE_NOT_READY,
    PS5_AUTH_LISTENER_STATE_INIT,
    PS5_AUTH_LISTENER_STATE_READY,
};

struct PS5AuthUSBListenerCache {
    uint8_t report_id;
    uint16_t report_len;
    uint8_t *buf;
};

class PS5AuthUSBListener : public USBListener {
public:
    ~PS5AuthUSBListener();
    void setup() override;
    void mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report,
               uint16_t desc_len) override;
    void xmount(uint8_t dev_addr, uint8_t instance, uint8_t controllerType,
                uint8_t subtype) override {}
    void unmount(uint8_t dev_addr) override;
    void report_received(uint8_t dev_addr, uint8_t instance,
                         const uint8_t *report_buf, uint16_t len) override {}
    void report_sent(uint8_t dev_addr, uint8_t instance,
                     const uint8_t *report_buf, uint16_t len) override {}
    void set_report_complete(uint8_t dev_addr, uint8_t instance,
                             uint8_t report_id, uint8_t report_type,
                             uint16_t len) override;
    void get_report_complete(uint8_t dev_addr, uint8_t instance,
                             uint8_t report_id, uint8_t report_type,
                             uint16_t len) override;
    void process(); // add things to process

    uint16_t get_cached_report(uint8_t report_id, void* report, uint16_t len);
    bool host_set_report(uint8_t report_id, const void* report, uint16_t len);

private:
    void init_cache_item();
    bool host_get_report(uint8_t report_id, void* report, uint16_t len);
    void reset();

    PS5AuthUSBListenerState m_state;
    std::vector<PS5AuthUSBListenerCache>::iterator m_init_item;
    uint8_t m_pending_report;
    uint8_t m_ps_dev_addr; // TinyUSB Address (USB)
    uint8_t m_ps_instance; // TinyUSB Instance (USB)

    std::vector<PS5AuthUSBListenerCache> m_cache {
        { DS_FEATURE_REPORT_CALIBRATION, DS_FEATURE_REPORT_CALIBRATION_SIZE, nullptr },
        { DS_FEATURE_REPORT_MACADDR, DS_FEATURE_REPORT_MACADDR_SIZE, nullptr },
        { DS_FEATURE_REPORT_FW_INFO, DS_FEATURE_REPORT_FW_INFO_SIZE, nullptr },
    };
};

#endif // _PS5AUTHUSBLISTENER_H_

