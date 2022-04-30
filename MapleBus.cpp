#include "MapleBus.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"
#include "configuration.h"
#include "maple.pio.h"

pio_hw_t* const MapleBus::PIO_OUT = pio0;
pio_hw_t* const MapleBus::PIO_IN = pio1;


uint MapleBus::getOutProgramOffset()
{
    static int offset = -1;
    if (offset < 0)
    {
        offset = pio_add_program(MapleBus::PIO_OUT, &maple_out_program);
    }
    return (uint)offset;
}

uint MapleBus::getInProgramOffset()
{
    static int offset = -1;
    if (offset < 0)
    {
        offset = pio_add_program(MapleBus::PIO_IN, &maple_in_program);
    }
    return (uint)offset;
}

MapleBus::MapleBus(uint32_t pinA, uint8_t senderAddr) :
    mPinA(pinA),
    mPinB(pinA + 1),
    mMaskA(1 << mPinA),
    mMaskB(1 << mPinB),
    mMaskAB(mMaskA | mMaskB),
    mSenderAddr(senderAddr),
    mPioOutData(PIO_OUT,
                getOutProgramOffset(),
                pio_claim_unused_sm(PIO_OUT, true),
                pio_maple_out_get_config(getOutProgramOffset(), CPU_FREQ_KHZ, MIN_CLOCK_PERIOD_NS, mPinA)),
    mPioInData(PIO_IN,
               getInProgramOffset(),
               pio_claim_unused_sm(PIO_IN, true),
               pio_maple_in_get_config(getInProgramOffset(), mPinA))
{
    pio_maple_out_pin_init(mPioOutData.pio, mPioOutData.smIdx, mPioOutData.programOffset, mPioOutData.config, mPinA);
    pio_maple_in_pin_init(mPioInData.pio, mPioInData.smIdx, mPioInData.programOffset, mPioInData.config, mPinA);
}

bool MapleBus::writeInit()
{
    const uint64_t targetTime = time_us_64() + OPEN_LINE_CHECK_TIME_US + 1;

    // Ensure no one is pulling low
    do
    {
        if ((gpio_get_all() & mMaskAB) != mMaskAB)
        {
            // Something is pulling low
            return false;
        }
    } while (time_us_64() < targetTime);

    pio_maple_out_start(mPioOutData.pio, mPioOutData.smIdx, mPinA);

    return true;
}

bool MapleBus::write(uint32_t frameWord, uint32_t* words, uint8_t len)
{
    bool rv = false;

    uint8_t crc = 0;
    swapByteOrder(mBuffer[0], frameWord, crc);
    for (uint32_t i = 0; i < len; ++i)
    {
        swapByteOrder(mBuffer[i + 1], words[i], crc);
    }
    mBuffer[len + 1] = crc << 24;

    if (writeInit())
    {
        // Send to FIFO!
        // First word is how many bits are going out
        uint32_t numBits = (len * 4 + 5) * 8;
        pio_sm_put_blocking(mPioOutData.pio, mPioOutData.smIdx, numBits);
        for (int i = 0; i < len + 2; ++i)
        {
            pio_sm_put_blocking(mPioOutData.pio, mPioOutData.smIdx, mBuffer[i]);
        }
        rv = true;
    }

    // For now, just sleep - need to implement ISR later
    sleep_us(300);

    pio_maple_out_stop(mPioOutData.pio, mPioOutData.smIdx);

    return rv;
}

bool MapleBus::write(uint32_t* words, uint8_t len)
{
    return write(words[0], words + 1, len - 1);
}

bool MapleBus::write(uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len)
{
    uint32_t frameWord = (len) | (mSenderAddr << 8) | (recipientAddr << 16) | (command << 24);
    return write(frameWord, words, len);
}

