#ifndef _PS5AUTH_H_
#define _PS5AUTH_H_

#include "drivers/shared/gpauthdriver.h"
#include "drivers/ps5/PS5AuthUSBListener.h"

class PS5Auth : public GPAuthDriver {
public:
    PS5Auth() {}
    void initialize() override;
    bool available() override;
    void process();
    void resetAuth();

    uint16_t get_report(uint8_t report_id, uint8_t *buffer,
                        uint16_t report_len);
    void set_report(uint8_t report_id, const uint8_t *buffer,
                    uint16_t report_len);

    void sign_hid(const void *report, uint16_t len);
};

#endif

