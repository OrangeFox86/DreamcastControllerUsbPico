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

// 13 is approximately the number of operations after the while loop that it takes to write bits and
// reset cvr
#define CLOCK_BIT_BIAS 13

// This is a bit imprecise, but it gets a better throughput than wasting cycles on an interrupt with
// all of the delays associated with that
inline void clockBit(const uint32_t& mask, const uint32_t& value)
{
    uint32_t toggle = (sio_hw->gpio_out ^ value) & mask;

    while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_BIAS));
    sio_hw->gpio_togl = toggle;
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
    clockBit(maskAB, maskAB);
    clockBit(maskAB, maskAB);
    clockBit(maskAB, maskAB);

    // Start
    clockBit(maskAB, maskB);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskB);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskB);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskB);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskB);
    clockBit(maskAB, maskAB);

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
                    clockBit(maskAB, maskAB);
                }
                lastState = maskB;
                clockBit(maskAB, lastState);
            }
            else
            {
                if (lastState != maskA)
                {
                    clockBit(maskAB, maskA);
                }
                lastState = 0;
                clockBit(maskAB, lastState);
            }

            mask = mask >> 1;

            // B is clock and A is data
            if (*bytePtr & mask)
            {
                if (lastState != maskAB)
                {
                    clockBit(maskAB, maskAB);
                }
                lastState = maskA;
                clockBit(maskAB, lastState);
            }
            else
            {
                if (lastState != maskB)
                {
                    clockBit(maskAB, maskB);
                }
                lastState = 0;
                clockBit(maskAB, lastState);
            }
        }
    }

    // End
    clockBit(maskAB, maskAB);
    clockBit(maskAB, maskA);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskA);
    clockBit(maskAB, 0);
    clockBit(maskAB, maskA);
    clockBit(maskAB, maskAB);

    mapleBus.writeComplete();
}

void clocking_init()
{
    systick_hw->csr = (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_ENABLE_MASK);
    systick_hw->rvr = SYSTICK_RELOAD_VALUE;
}
