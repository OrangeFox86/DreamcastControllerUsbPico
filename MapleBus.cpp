#include "MapleBus.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"
#include "configuration.h"

MapleBus::MapleBus(uint32_t pinA, uint32_t pinB, uint8_t senderAddr) :
    mLastPut(0),
    mPinA(pinA),
    mPinB(pinB),
    mMaskA(1 << pinA),
    mMaskB(1 << pinB),
    mMaskAB(mMaskA | mMaskB),
    mSenderAddr(senderAddr),
    mCrc(0)
{
    // Initialize the pins as inputs with pull ups so that idle line will always be high
    gpio_init(mPinA);
    gpio_init(mPinB);
    gpio_set_dir_in_masked(mMaskAB);
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);
}

// This is a bit imprecise, but it gets a better throughput than wasting cycles on an
// interrupt with all of the delays associated with that
inline void MapleBus::putAB(const uint32_t& value)
{
    // Don't waste time setting this bit if we're already there
    if (value != mLastPut)
    {
        mLastPut = value;
        // Compute the bits we'd like to toggle
        uint32_t toggle = (sio_hw->gpio_out ^ value) & mMaskAB;
        // Wait for next clock tick
        delay();
        // Send out the bits
        sio_hw->gpio_togl = toggle;
        // Reset systick for next put
        systick_hw->cvr = 0;
        // In one of my tests, COUNTFLAG got set with the above CVR write (instead of cleared as
        // documented). I haven't seen that issue since, but just in case, the following should
        // force it to be cleared.
        (void)systick_hw->csr;
    }
}

inline void MapleBus::toggleA()
{
    // Same as putAB, but just toggle A
    mLastPut ^= mMaskA;
    delay();
    sio_hw->gpio_togl = mMaskA;
    systick_hw->cvr = 0;
    (void)systick_hw->csr;
}

inline void MapleBus::toggleB()
{
    // Same as putAB, but just toggle B
    mLastPut ^= mMaskB;
    delay();
    sio_hw->gpio_togl = mMaskB;
    systick_hw->cvr = 0;
    (void)systick_hw->csr;
}

inline void MapleBus::delay()
{
    // Wait for systick to count down to 0
    while(!(systick_hw->csr & M0PLUS_SYST_CSR_COUNTFLAG_BITS));
}

bool MapleBus::writeInit()
{
    mCrc = 0;
    // Make sure the clock is turned on
    systick_hw->csr = (M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_ENABLE_BITS);
    systick_hw->rvr = SYSTICK_RELOAD_VALUE;
    // Ensure no one is pulling low
    if ((gpio_get_all() & mMaskAB) == mMaskAB)
    {
        // Set the two pins at output and keep HIGH
        // If I could, I would keep HIGH using pullups and only switch to output when LOW is needed,
        // but I/O switching doesn't happen as fast as switching levels as output.
        gpio_put_masked(mMaskAB, mMaskAB);
        gpio_set_dir_out_masked(mMaskAB);
        gpio_put_masked(mMaskAB, mMaskAB);
        return true;
    }
    else
    {
        return false;
    }
}

void MapleBus::writeComplete()
{
    // Make A and B inputs once again (pull-ups still active)
    gpio_set_dir_in_masked(mMaskAB);
}

void MapleBus::writeStartSequence()
{
    // Ensure it's at neutral for a few cycles
    putAB(mMaskAB);
    delay();
    delay();

    // Start
    toggleA();
    toggleB();
    toggleB();
    toggleB();
    toggleB();
    toggleB();
    toggleB();
    toggleB();
    toggleB();
    toggleA();
    toggleB();
}

void MapleBus::writeEndSequence()
{
    // End
    putAB(mMaskAB);
    toggleB();
    toggleA();
    toggleA();
    toggleA();
    toggleA();
    toggleB();
}

inline void MapleBus::writeByte(const uint8_t& byte)
{
    for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1)
    {
        // A is clock and B is data
        if (byte & mask)
        {
            putAB(mMaskAB);
        }
        else
        {
            putAB(mMaskA);
        }
        toggleA();

        mask = mask >> 1;

        // B is clock and A is data
        if (byte & mask)
        {
            putAB(mMaskAB);
        }
        else
        {
            putAB(mMaskB);
        }
        toggleB();
    }
    mCrc ^= byte;
}

bool MapleBus::write(uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len)
{
    bool rv = false;
    // assuming this is running little-endian already
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(words);
    uint8_t* const endPtr = bytePtr + (len * sizeof(*words));

    if (writeInit())
    {
        // Start
        writeStartSequence();

        // Frame word
        writeByte(len);
        writeByte(mSenderAddr);
        writeByte(recipientAddr);
        writeByte(command);

        // Payload
        for (; bytePtr < endPtr; ++bytePtr)
        {
            writeByte(*bytePtr);
        }

        // CRC
        writeByte(mCrc);

        // End
        writeEndSequence();

        writeComplete();

        rv = true;
    }
    return rv;
}
