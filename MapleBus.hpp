#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "configuration.h"

class MapleBus
{
    public:
        MapleBus(uint32_t pinA, uint32_t pinB);
        void write(uint8_t* bytes, uint32_t len);

    private:

        bool writeInit() const;
        void writeComplete() const;

        // This is a bit imprecise, but it gets a better throughput than wasting cycles on an
        // interrupt with all of the delays associated with that
        inline void putAB(const uint32_t& value)
        {
            uint32_t toggle = (sio_hw->gpio_out ^ value) & mMaskAB;

            while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_BIAS));
            sio_hw->gpio_togl = toggle;
            systick_hw->cvr = 0;
        }

        const uint32_t mPinA;
        const uint32_t mPinB;
        const uint32_t mMaskA;
        const uint32_t mMaskB;
        const uint32_t mMaskAB;

        // Nominal reload value, not account for any operation adjustment
        static const uint32_t SYSTICK_RELOAD_VALUE = 0x00FFFFFF;
        // Nominal threshold, not accounting for any operation adjustment
        static const uint32_t SYSTICK_NOMINAL_THRESHOLD = (SYSTICK_RELOAD_VALUE - CPU_TICKS_PER_PERIOD - 1);
        // 11 is approximately the number of operations after the while loop that it takes to write
        // bits and reset cvr
        static const uint32_t CLOCK_BIT_BIAS = 11;
};

#endif // __MAPLE_BUS_H__
