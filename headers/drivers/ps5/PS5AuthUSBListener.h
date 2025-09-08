#ifndef _PS5AUTHUSBLISTENER_H_
#define _PS5AUTHUSBLISTENER_H_

#include "usblistener.h"
#include "PS5Driver.h"
#include "PS5Auth.h"

class PS5AuthUSBListener : public USBListener {
public:
    void setup() override;
    void mount(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) override;
    void xmount(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) override {}
    void unmount(uint8_t dev_addr) override;
    void report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) override {}
    void report_sent(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) override {}
    void set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) override;
    void get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) override;
    void process(); // add things to process
    void resetHostData();
private:
    bool host_get_report(uint8_t report_id, void* report, uint16_t len);
    bool host_set_report(uint8_t report_id, void* report, uint16_t len);
    bool awaiting_cb;   // Global call-back wait
    uint8_t ps_dev_addr; // TinyUSB Address (USB)
    uint8_t ps_instance; // TinyUSB Instance (USB)
};

#endif // _PS5AUTHUSBLISTENER_H_

