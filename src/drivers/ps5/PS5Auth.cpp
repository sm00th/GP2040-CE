#include "drivers/ps5/PS5Auth.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include <iostream>

void PS5Auth::initialize() {
    listener = new PS5AuthUSBListener();
    ((PS5AuthUSBListener*)listener)->setup();
    printf("ps5_auth %s\n", __func__);
}

bool PS5Auth::available() {
    return true;
}

void PS5Auth::process() {
}

void PS5Auth::resetAuth() {
}
