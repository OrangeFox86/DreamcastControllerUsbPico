#include "MapleBus.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
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

// 11 is approximately the number of operations after the while loop that it takes to write bits and
// reset cvr
#define CLOCK_BIT_BIAS 11

static inline void initClock()
{
    systick_hw->csr = (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_ENABLE_MASK);
    systick_hw->rvr = SYSTICK_RELOAD_VALUE;
}

// This is a bit imprecise, but it gets a better throughput than wasting cycles on an interrupt with
// all of the delays associated with that
static inline void clockBit(const uint32_t& mask, const uint32_t& value)
{
    uint32_t toggle = (sio_hw->gpio_out ^ value) & mask;

    while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_BIAS));
    sio_hw->gpio_togl = toggle;
    systick_hw->cvr = 0;
}

MapleBus::MapleBus(uint32_t pinA, uint32_t pinB) :
    mPinA(pinA),
    mPinB(pinB),
    mPinAMask(1 << pinA),
    mPinBMask(1 << pinB),
    mAllPinMask(mPinAMask | mPinBMask)
{
    // B must be the very next output from A
    assert((pinB - pinA) == 1);
    // Initialize the pins as inputs with pull ups so that idle line will always be high
    gpio_init(mPinA);
    gpio_init(mPinB);
    gpio_set_dir_in_masked(mAllPinMask);
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}

bool MapleBus::writeInit() const
{
    // Make sure the clock is turned on
    initClock();
    // Only one pair may be output, so make all others input
    gpio_set_dir_in_masked(~mAllPinMask);
    // Ensure no one is pulling low
    if ((gpio_get_all() & mAllPinMask) == mAllPinMask)
    {
        // Set the two pins at output and keep HIGH
        gpio_put_all(mAllPinMask);
        gpio_set_dir_out_masked(mAllPinMask);
        gpio_put_all(mAllPinMask);
        return true;
    }
    else
    {
        return false;
    }
}

void MapleBus::writeComplete() const
{
    // Make everything input
    gpio_set_dir_in_masked(0xFFFFFFFF);
    // Pull up setting shouldn't have changed, but just for good measure...
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}

void MapleBus::write(uint8_t* bytes, uint32_t len)
{
    uint8_t* bytePtr = bytes;
    uint8_t* const endPtr = bytes + len;
    const uint32_t maskA = MASK_A << mPinA;
    const uint32_t maskB = MASK_B << mPinA;
    const uint32_t maskAB = MASK_AB << mPinA;

    writeInit();

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

    writeComplete();
}