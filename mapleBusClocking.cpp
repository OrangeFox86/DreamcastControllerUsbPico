#include "pico/stdlib.h"
#include "hardware/structs/systick.h"

#include "mapleBusClocking.hpp"
#include "MapleBus.hpp"
#include "configuration.h"

// From datasheet
#define SYST_CSR_CLKSOURCE_MASK 0x00000004 // 1 for processor; 0 for external
#define SYST_CSR_TICKINT_MASK   0x00000002 // 1 enables isr_systick; 0 disables
#define SYST_CSR_ENABLE_MASK    0x00000001 // 1 enables counter; 0 disables

// Nominal reload value, not account for any operation adjustment
#define SYSTICK_RELOAD_VALUE 0x00FFFFFF
// Nominal threshold, not accounting for any operation adjustment
#define SYSTICK_NOMINAL_THRESHOLD (SYSTICK_RELOAD_VALUE - CPU_TICKS_PER_PERIOD - 1)

// Mask values for sending A and B bits
#define MASK_A 0x01
#define MASK_B 0x02
#define MASK_AB (MASK_A | MASK_B)

// 13 is approximately the number of operations after the while loop that it takes to write and
// reset cvr
#define CLOCK_BIT_BIAS 13

// This is a bit imprecise, but it gets a better throughput than wasting cycles on an interrupt with
// all of the delays associated with that
inline void clockBit(const uint32_t& value)
{
    while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_BIAS));
    sio_hw->gpio_out = value;
    systick_hw->cvr = 0;
}

// Clock slightly faster for some operations
#define CLOCK_BIT_FAST_BIAS (CLOCK_BIT_BIAS + 2)

inline void clockBitFast(const uint32_t& value)
{
    while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_FAST_BIAS));
    sio_hw->gpio_out = value;
    systick_hw->cvr = 0;
}

void write(const MapleBus& mapleBus, uint8_t* bytes, uint32_t len)
{
    uint8_t* bytePtr = bytes;
    uint8_t* const endPtr = bytes + len;
    const uint32_t maskA = MASK_A << mapleBus.mPinA;
    const uint32_t maskB = MASK_B << mapleBus.mPinA;
    const uint32_t maskAB = MASK_AB << mapleBus.mPinA;

    mapleBus.writeInit();

    // Ensure it's at neutral for a few cycles
    clockBit(maskAB);
    clockBit(maskAB);
    clockBit(maskAB);

    // Start - I think it's good for the start clocks to be slightly faster
    //         Slower devices that may be close to tolerance will fail to capture this
    clockBitFast(maskB);
    clockBitFast(0);
    clockBitFast(maskB);
    clockBitFast(0);
    clockBitFast(maskB);
    clockBitFast(0);
    clockBitFast(maskB);
    clockBitFast(0);
    clockBitFast(maskB);
    clockBitFast(maskAB);

    // Data
    uint32_t lastState = (maskAB);
    for (; bytePtr < endPtr; ++bytePtr)
    {
        for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1)
        {
            // A is clock and B is data
            if (*bytePtr & mask)
            {
                if (lastState != maskAB)
                {
                    clockBit(maskAB);
                }
                lastState = maskB;
                clockBit(lastState);
            }
            else
            {
                if (lastState != maskA)
                {
                    clockBit(maskA);
                }
                lastState = 0;
                clockBit(lastState);
            }

            mask = mask >> 1;

            // B is clock and A is data
            if (*bytePtr & mask)
            {
                if (lastState != maskAB)
                {
                    clockBit(maskAB);
                }
                lastState = maskA;
                clockBit(lastState);
            }
            else
            {
                if (lastState != maskB)
                {
                    clockBit(maskB);
                }
                lastState = 0;
                clockBit(lastState);
            }
        }
    }

    // End
    clockBit(maskAB);
    clockBit(maskA);
    clockBit(0);
    clockBit(maskA);
    clockBit(0);
    clockBit(maskA);
    clockBit(maskAB);

    mapleBus.writeComplete();
}

void clocking_init()
{
    systick_hw->csr = (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_ENABLE_MASK);
    systick_hw->rvr = SYSTICK_RELOAD_VALUE;
}
