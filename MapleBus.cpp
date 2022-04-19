#include "MapleBus.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "configuration.h"

// From datasheet
#define SYST_CSR_CLKSOURCE_MASK 0x00000004 // 1 for processor; 0 for external
#define SYST_CSR_TICKINT_MASK   0x00000002 // 1 enables isr_systick; 0 disables
#define SYST_CSR_ENABLE_MASK    0x00000001 // 1 enables counter; 0 disables

// Mask values for sending A and B bits
#define MASK_A 0x01
#define MASK_B 0x02
#define MASK_AB (MASK_A | MASK_B)


// This is a bit imprecise, but it gets a better throughput than wasting cycles on an
// interrupt with all of the delays associated with that
void MapleBus::putAB(const uint32_t& value)
{
    uint32_t toggle = (sio_hw->gpio_out ^ value) & mMaskAB;

    while(systick_hw->cvr > (SYSTICK_NOMINAL_THRESHOLD + CLOCK_BIT_BIAS));
    sio_hw->gpio_togl = toggle;
    systick_hw->cvr = 0;
}

MapleBus::MapleBus(uint32_t pinA, uint32_t pinB) :
    mPinA(pinA),
    mPinB(pinB),
    mMaskA(1 << pinA),
    mMaskB(1 << pinB),
    mMaskAB(mMaskA | mMaskB)
{
    // B must be the very next output from A
    assert((pinB - pinA) == 1);
    // Initialize the pins as inputs with pull ups so that idle line will always be high
    gpio_init(mPinA);
    gpio_init(mPinB);
    gpio_set_dir_in_masked(mMaskAB);
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}

bool MapleBus::writeInit() const
{
    // Make sure the clock is turned on
    systick_hw->csr = (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_ENABLE_MASK);
    systick_hw->rvr = SYSTICK_RELOAD_VALUE;
    // Only one pair may be output, so make all others input
    gpio_set_dir_in_masked(~mMaskAB);
    // Ensure no one is pulling low
    if ((gpio_get_all() & mMaskAB) == mMaskAB)
    {
        // Set the two pins at output and keep HIGH
        gpio_put_all(mMaskAB);
        gpio_set_dir_out_masked(mMaskAB);
        gpio_put_all(mMaskAB);
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

    writeInit();

    // Ensure it's at neutral for a few cycles
    putAB(mMaskAB);
    putAB(mMaskAB);
    putAB(mMaskAB);

    // Start
    putAB(mMaskB);
    putAB(0);
    putAB(mMaskB);
    putAB(0);
    putAB(mMaskB);
    putAB(0);
    putAB(mMaskB);
    putAB(0);
    putAB(mMaskB);
    putAB(mMaskAB);

    // Data
    uint32_t lastState = mMaskAB;
    for (; bytePtr < endPtr; ++bytePtr)
    {
        for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1)
        {
            // A is clock and B is data
            if (*bytePtr & mask)
            {
                if (lastState != mMaskAB)
                {
                    putAB(mMaskAB);
                }
                lastState = mMaskB;
                putAB(lastState);
            }
            else
            {
                if (lastState != mMaskA)
                {
                    putAB(mMaskA);
                }
                lastState = 0;
                putAB(lastState);
            }

            mask = mask >> 1;

            // B is clock and A is data
            if (*bytePtr & mask)
            {
                if (lastState != mMaskAB)
                {
                    putAB(mMaskAB);
                }
                lastState = mMaskA;
                putAB(lastState);
            }
            else
            {
                if (lastState != mMaskB)
                {
                    putAB(mMaskB);
                }
                lastState = 0;
                putAB(lastState);
            }
        }
    }

    // End
    putAB(mMaskAB);
    putAB(mMaskA);
    putAB(0);
    putAB(mMaskA);
    putAB(0);
    putAB(mMaskA);
    putAB(mMaskAB);

    writeComplete();
}