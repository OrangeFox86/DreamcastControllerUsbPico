#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "configuration.h"

class MapleBus
{
    public:
        MapleBus(uint32_t pinA, uint32_t pinB, uint8_t senderAddr);
        bool write(uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len);


    private:

        bool writeInit();
        void writeComplete();

        void writeStartSequence();
        void writeEndSequence();
        void writeByte(const uint8_t& byte);

        void putAB(const uint32_t& value);

        uint32_t mLastPut;
        const uint32_t mPinA;
        const uint32_t mPinB;
        const uint32_t mMaskA;
        const uint32_t mMaskB;
        const uint32_t mMaskAB;
        const uint8_t mSenderAddr;
        uint32_t mCrc;

        // Reload value for systick
        static const uint32_t SYSTICK_RELOAD_VALUE = 0x00FFFFFF;
        // 11 is approximately the number of operations after the while loop that it takes to write
        // bits and reset cvr
        static const uint32_t CLOCK_BIT_BIAS = 11;
        // Threshold that we wait for systick to pass before putting bits out
        static const uint32_t SYSTICK_THRESHOLD = (SYSTICK_RELOAD_VALUE - CPU_TICKS_PER_PERIOD - 1 + CLOCK_BIT_BIAS);
};

#endif // __MAPLE_BUS_H__
