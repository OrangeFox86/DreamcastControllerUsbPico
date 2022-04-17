#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"

class MapleBus
{
    public:
        MapleBus(uint32_t pinA, uint32_t pinB);

        bool writeInit() const;

        void writeComplete() const;

        const uint32_t mPinA;
        const uint32_t mPinB;
        const uint32_t mPinAMask;
        const uint32_t mPinBMask;
        const uint32_t mAllPinMask;
};

#endif // __MAPLE_BUS_H__
