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
    mLastPut = 0;

    // Make sure the clock is turned on and set for writing
    systick_hw->csr = (M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_ENABLE_BITS);
    systick_hw->rvr = SYSTICK_WRITE_RELOAD_VALUE;
    systick_hw->cvr = 0;

    // Ensure no one is pulling low
    uint32_t count = OPEN_LINE_CHECK_CYCLES;
    while (count != 0)
    {
        if ((gpio_get_all() & mMaskAB) != mMaskAB)
        {
            // Something is pulling low
            return false;
        }

        // Every time systick overflows, update count
        if (systick_hw->csr & M0PLUS_SYST_CSR_COUNTFLAG_BITS)
        {
            --count;
        }
    }

    // Set the two pins at output and keep HIGH
    gpio_put_masked(mMaskAB, mMaskAB);
    gpio_set_dir_out_masked(mMaskAB);
    gpio_put_masked(mMaskAB, mMaskAB);
    return true;
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
    mCrc ^= byte;
    uint8_t mask = 0x80;
    do
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

        mask = mask >> 1;
    } while (mask != 0);
}

inline void MapleBus::writeBytes(const uint8_t* bytePtr, const uint8_t* endPtr)
{
    while(bytePtr != endPtr)
    {
        writeByte(*bytePtr);
        ++bytePtr;
    }
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
        writeBytes(bytePtr, endPtr);

        // CRC
        writeByte(mCrc);

        // End
        writeEndSequence();

        writeComplete();

        rv = true;
    }
    return rv;
}

// Storage used for buffering read states from GPIO inputs
static uint32_t readBuffer[READ_BUFFER_SIZE];

// Since this will block until complete anyway, it's faster to poll than it is to rely on
// interrupt with all the delays associated with that.
bool MapleBus::read(uint32_t* words, uint32_t& len)
{
    uint32_t lastRead = mMaskAB;
    uint32_t* pReads = readBuffer;
    uint32_t* pReadsEnd = readBuffer + READ_BUFFER_SIZE;
    uint32_t read = 0;

    // Make sure the clock is turned on and set for reading
    systick_hw->csr = (M0PLUS_SYST_CSR_CLKSOURCE_BITS | M0PLUS_SYST_CSR_ENABLE_BITS);
    systick_hw->rvr = SYSTICK_READ_RELOAD_VALUE;
    systick_hw->cvr = 0;
    (void)systick_hw->csr; // not necessary, but it makes me happy

    // There's barely enough time to read
    // Just 5 more nops in this loop will cause it to fail much more frequently
    while(true)
    {
        // No masking done here for simplicity. As a result, no other input operations may exist on
        // the microcontroller at the same time.
        read = sio_hw->gpio_in;

        if (read != lastRead && pReads < pReadsEnd)
        {
            *pReads++ = read;
            lastRead = read;
        }

        // If systick overflows, we're done reading
        // This needs to be done in "else if" in order to speed things up a bit
        else if (systick_hw->csr & M0PLUS_SYST_CSR_COUNTFLAG_BITS)
        {
            break;
        }
    }

    // Concatenate and reset pointer
    pReadsEnd = pReads;
    pReads = readBuffer;

    uint8_t bitMask = 0x80;
    uint8_t byte = 0;
    uint32_t changed = 0;

    // assuming this is running little-endian already
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(words);
    uint8_t* const endPtr = bytePtr + (len * sizeof(*words));
    len = 0;

    // Wait for start or error
    while(true)
    {
        if (pReads == pReadsEnd)
        {
            return false;
        }

        read = *pReads++ & mMaskAB;

        // Simplified this down to just waiting until A is high and B is low; this should be the
        // start of data as long as no noise is on the line
        if (read == mMaskA)
        {
            break;
        }
    }

    // Parse data
    lastRead = read;
    bitMask = 0x80;
    byte = 0;
    uint8_t expectedCrc = 0;
    uint8_t readCrc = 0;
    int_fast16_t expectedByteCount = -1;
    while(true)
    {
        if (pReads == pReadsEnd)
        {
            return false;
        }

        read = *pReads++;
        changed = lastRead ^ read;
        lastRead = read;
        if ((changed & mMaskA) && !(read & mMaskA))
        {
            // A went low
            if (read & mMaskB)
            {
                byte |= bitMask;
            }

            if (bitMask & 0x55)
            {
                // Invalid clock sequence
                return false;
            }

            bitMask = bitMask >> 1;
        }
        else if ((changed & mMaskB) && !(read & mMaskB))
        {
            // B went low
            if (read & mMaskA)
            {
                byte |= bitMask;
            }

            if (bitMask & 0xAA)
            {
                // Invalid clock sequence
                return false;
            }
            else if (bitMask == 1)
            {
                if (expectedByteCount < 0)
                {
                    // First byte is number of words after frame
                    // Frame word * 4 + num words * 4 + crc byte * 1
                    // Then subtract 1 for this word
                    expectedByteCount = 4 + (byte * 4) + 1 - 1;
                }
                else
                {
                    --expectedByteCount;
                }

                if (expectedByteCount == 0)
                {
                    readCrc = byte;
                    break;
                }
                else if (bytePtr < endPtr)
                {
                    expectedCrc ^= byte;
                    *bytePtr = byte;
                    byte = 0;
                    ++bytePtr;
                }
                else
                {
                    // Overflow
                    len = len + 1;
                    return false;
                }

                bitMask = 0x80;
            }
            else
            {
                bitMask = bitMask >> 1;
            }
        }
    }

    // For simplicity, this doesn't even verify an end sequence

    len = (bytePtr - reinterpret_cast<uint8_t*>(words)) / 4;

    return (expectedCrc == readCrc);
}
