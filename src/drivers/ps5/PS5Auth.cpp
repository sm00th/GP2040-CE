#include "drivers/ps5/PS5Auth.h"
#include "peripheralmanager.h"
#include <iostream>

void PS5Auth::initialize() {
    if ( !available() ) {
        return;
    }

    listener = new PS5AuthUSBListener();
    static_cast<PS5AuthUSBListener*>(listener)->setup();
    printf("ps5_auth %s\n", __func__);
}

bool PS5Auth::available() {
    printf("%s: %d\n", __func__, PeripheralManager::getInstance().isUSBEnabled(0));
    return (PeripheralManager::getInstance().isUSBEnabled(0));
}

uint16_t PS5Auth::get_report(uint8_t report_id, uint8_t *buffer,
                         uint16_t report_len) {
    printf("ps5_auth %s: 0x%02x (%d)\n", __func__, report_id, report_len);
    return static_cast<PS5AuthUSBListener*>(listener)->get_cached_report(report_id, buffer, report_len);
}

void PS5Auth::set_report(uint8_t report_id, const uint8_t *buffer,
                         uint16_t report_len) {
    printf("ps5_auth %s: 0x%02x (%d)\n", __func__, report_id, report_len);
    static_cast<PS5AuthUSBListener*>(listener)->host_set_report(report_id, buffer, report_len);
}

void PS5Auth::sign_hid(const void *buffer, uint16_t report_len) {
    printf("ps5_auth %s: %d\n", __func__, report_len);
    static_cast<PS5AuthUSBListener*>(listener)->send_hid_report(0, buffer, report_len);
}

void PS5Auth::process() {
}

void PS5Auth::resetAuth() {
}
