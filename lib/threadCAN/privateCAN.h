#ifndef PRIVATE_CAN_H
#define PRIVATE_CAN_H

#include "mbed.h"

class PrivateCAN : public CAN {
public:
    PrivateCAN(PinName rd, PinName td) : CAN(rd, td) {}
    PrivateCAN(PinName rd, PinName td, int hz) : CAN(rd, td, hz) {}
protected:
    virtual void lock() {}
    virtual void unlock() {}
};

#endif
