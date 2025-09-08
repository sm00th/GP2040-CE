#ifndef _PS5AUTH_H_
#define _PS5AUTH_H_

#include "drivers/shared/gpauthdriver.h"

class PS5Auth : public GPAuthDriver {
public:
    PS5Auth() {}
    virtual void initialize();
    virtual bool available();
    void process();
    void resetAuth();
};

#endif

