#include "drivers/ps5/PS5Auth.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include "peripheralmanager.h"
#include <iostream>

void PS5Auth::initialize() {
    if ( !available() ) {
        return;
    }

    listener = new PS5AuthUSBListener();
    ((PS5AuthUSBListener*)listener)->setup();
    printf("ps5_auth %s\n", __func__);
}

bool PS5Auth::available() {
    printf("%s: %d\n", __func__, PeripheralManager::getInstance().isUSBEnabled(0));
    return (PeripheralManager::getInstance().isUSBEnabled(0));
}

void PS5Auth::process() {
}

void PS5Auth::resetAuth() {
}
