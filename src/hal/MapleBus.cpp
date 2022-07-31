#include "MapleBus.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"
#include "hardware/irq.h"
#include "configuration.h"
#include "maple.pio.h"
#include "string.h"

MapleBus* mapleWriteIsr[4] = {};
MapleBus* mapleReadIsr[4] = {};

extern "C"
{
void maple_write_isr0(void)
{
    if (MAPLE_OUT_PIO->irq & (0x01))
    {
        mapleWriteIsr[0]->writeIsr();
        hw_set_bits(&MAPLE_OUT_PIO->irq, 0x01);
    }
    if (MAPLE_OUT_PIO->irq & (0x04))
    {
        mapleWriteIsr[2]->writeIsr();
        hw_set_bits(&MAPLE_OUT_PIO->irq, 0x04);
    }
}
void maple_write_isr1(void)
{
    if (MAPLE_OUT_PIO->irq & (0x02))
    {
        mapleWriteIsr[1]->writeIsr();
        hw_set_bits(&MAPLE_OUT_PIO->irq, 0x02);
    }
    if (MAPLE_OUT_PIO->irq & (0x08))
    {
        mapleWriteIsr[3]->writeIsr();
        hw_set_bits(&MAPLE_OUT_PIO->irq, 0x08);
    }
}
void maple_read_isr0(void)
{
    if (MAPLE_IN_PIO->irq & (0x01))
    {
        mapleReadIsr[0]->readIsr();
        hw_set_bits(&MAPLE_IN_PIO->irq, 0x01);
    }
    if (MAPLE_IN_PIO->irq & (0x04))
    {
        mapleReadIsr[2]->readIsr();
        hw_set_bits(&MAPLE_IN_PIO->irq, 0x04);
    }
}
void maple_read_isr1(void)
{
    if (MAPLE_IN_PIO->irq & (0x02))
    {
        mapleReadIsr[1]->readIsr();
        hw_set_bits(&MAPLE_IN_PIO->irq, 0x02);
    }
    if (MAPLE_IN_PIO->irq & (0x08))
    {
        mapleReadIsr[3]->readIsr();
        hw_set_bits(&MAPLE_IN_PIO->irq, 0x08);
    }
}
}

void MapleBus::initIsrs()
{
    uint outIdx = pio_get_index(MAPLE_OUT_PIO);
    irq_set_exclusive_handler(PIO0_IRQ_0 + (outIdx * 2), maple_write_isr0);
    irq_set_exclusive_handler(PIO0_IRQ_1 + (outIdx * 2), maple_write_isr1);
    irq_set_enabled(PIO0_IRQ_0 + (outIdx * 2), true);
    irq_set_enabled(PIO0_IRQ_1 + (outIdx * 2), true);
    pio_set_irq0_source_enabled(MAPLE_OUT_PIO, pis_interrupt0, true);
    pio_set_irq1_source_enabled(MAPLE_OUT_PIO, pis_interrupt1, true);
    pio_set_irq0_source_enabled(MAPLE_OUT_PIO, pis_interrupt2, true);
    pio_set_irq1_source_enabled(MAPLE_OUT_PIO, pis_interrupt3, true);

    uint inIdx = pio_get_index(MAPLE_IN_PIO);
    irq_set_exclusive_handler(PIO0_IRQ_0 + (inIdx * 2), maple_read_isr0);
    irq_set_exclusive_handler(PIO0_IRQ_1 + (inIdx * 2), maple_read_isr1);
    irq_set_enabled(PIO0_IRQ_0 + (inIdx * 2), true);
    irq_set_enabled(PIO0_IRQ_1 + (inIdx * 2), true);
    pio_set_irq0_source_enabled(MAPLE_IN_PIO, pis_interrupt0, true);
    pio_set_irq1_source_enabled(MAPLE_IN_PIO, pis_interrupt1, true);
    pio_set_irq0_source_enabled(MAPLE_IN_PIO, pis_interrupt2, true);
    pio_set_irq1_source_enabled(MAPLE_IN_PIO, pis_interrupt3, true);
}

MapleBus::MapleBus(uint32_t pinA, uint8_t senderAddr) :
    mPinA(pinA),
    mPinB(pinA + 1),
    mMaskA(1 << mPinA),
    mMaskB(1 << mPinB),
    mMaskAB(mMaskA | mMaskB),
    mSenderAddr(senderAddr),
    mSmOut(CPU_FREQ_KHZ, MAPLE_NS_PER_BIT, mPinA),
    mSmIn(mPinA),
    mDmaWriteChannel(dma_claim_unused_channel(true)),
    mDmaReadChannel(dma_claim_unused_channel(true)),
    mWriteBuffer(),
    mReadBuffer(),
    mLastValidRead(),
    mLastValidReadLen(0),
    mNewDataAvailable(false),
    mReadUpdated(false),
    mWriteInProgress(false),
    mExpectingResponse(false),
    mReadInProgress(false),
    mProcKillTime(0xFFFFFFFFFFFFFFFFULL),
    mRxDetected(false),
    mReadTimeoutUs(0)
{
    mapleWriteIsr[mSmOut.mSmIdx] = this;
    mapleReadIsr[mSmIn.mSmIdx] = this;

    // This only needs to be called once but no issue calling it for each
    initIsrs();

    // Setup DMA to automaticlly put data on the FIFO
    dma_channel_config c = dma_channel_get_default_config(mDmaWriteChannel);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    // Bytes are instead manually swapped since CRC needs to be computed anyway
    // channel_config_set_bswap(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(mSmOut.mProgram.mPio, mSmOut.mSmIdx, true));
    dma_channel_configure(mDmaWriteChannel,
                            &c,
                            &mSmOut.mProgram.mPio->txf[mSmOut.mSmIdx],
                            mWriteBuffer,
                            sizeof(mWriteBuffer) / sizeof(mWriteBuffer[0]),
                            false);

    // Setup DMA to automaticlly read data from the FIFO
    c = dma_channel_get_default_config(mDmaReadChannel);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    // Bytes are instead manually swapped since CRC needs to be computed anyway
    // channel_config_set_bswap(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(mSmIn.mProgram.mPio, mSmIn.mSmIdx, false));
    dma_channel_configure(mDmaReadChannel,
                            &c,
                            mReadBuffer,
                            &mSmIn.mProgram.mPio->rxf[mSmIn.mSmIdx],
                            (sizeof(mReadBuffer) / sizeof(mReadBuffer[0])),
                            false);
}

inline void MapleBus::readIsr()
{
    if (!mRxDetected)
    {
        mRxDetected = true;
        mProcKillTime = time_us_64() + mReadTimeoutUs;
    }
    else
    {
        mSmIn.stop();
        mReadInProgress = false;
        mReadUpdated = true;
    }
}

inline void MapleBus::writeIsr()
{
    mSmOut.stop();
    if (mExpectingResponse)
    {
        mSmIn.start();
        mProcKillTime = time_us_64() + MAPLE_RESPONSE_TIMEOUT_US;
        mReadInProgress = true;
    }
    mWriteInProgress = false;
}

bool MapleBus::writeInit()
{
    const uint64_t targetTime = time_us_64() + MAPLE_OPEN_LINE_CHECK_TIME_US + 1;

    // Ensure no one is pulling low
    do
    {
        if ((gpio_get_all() & mMaskAB) != mMaskAB)
        {
            // Something is pulling low
            return false;
        }
    } while (time_us_64() < targetTime);

    mSmOut.start();

    return true;
}

bool MapleBus::write(const MaplePacket& packet,
                     bool expectResponse,
                     uint32_t readTimeoutUs)
{
    bool rv = false;

    processEvents();

    if (!isBusy() && packet.isValid())
    {
        // Make sure previous DMA instances are killed
        dma_channel_abort(mDmaWriteChannel);
        dma_channel_abort(mDmaReadChannel);

        mRxDetected = false;
        mReadTimeoutUs = readTimeoutUs;

        // First 32 bits sent to the state machine is how many bits to output.
        uint8_t len = packet.payload.size();
        uint32_t numBits = (len * 4 + 5) * 8;
        mWriteBuffer[0] = numBits;
        // The PIO state machine reads from "left to right" to achieve the right bit order, but the data
        // out needs to be little endian. Therefore, the data bytes needs to be swapped. Might as well
        // Compute the CRC while we're at it.
        uint8_t crc = 0;
        swapByteOrder(mWriteBuffer[1], packet.getFrameWord(mSenderAddr), crc);
        volatile uint32_t* pDest = &mWriteBuffer[2];
        for (std::vector<uint32_t>::const_iterator iter = packet.payload.begin();
             iter != packet.payload.end();
             ++iter, ++pDest)
        {
            swapByteOrder(*pDest, *iter, crc);
        }
        // Last byte left shifted out is the CRC
        mWriteBuffer[len + 2] = crc << 24;

        if (writeInit())
        {
            // Update flags before beginning to write
            mExpectingResponse = expectResponse;
            mWriteInProgress = true;

            if (expectResponse)
            {
                // Flush the read buffer
                updateLastValidReadBuffer();
                // Clear out the read buffer for good measure
                for (uint32_t i = 0; i < sizeof(mReadBuffer) / sizeof(mReadBuffer[0]); ++i)
                {
                    mReadBuffer[i] = 0;
                }
                // Start reading
                dma_channel_transfer_to_buffer_now(
                    mDmaReadChannel, mReadBuffer, sizeof(mReadBuffer) / sizeof(mReadBuffer[0]));
            }

            // Start writing
            dma_channel_transfer_from_buffer_now(mDmaWriteChannel, mWriteBuffer, len + 3);

            uint32_t totalWriteTimeNs = numBits * MAPLE_NS_PER_BIT;
            // Start and stop sequence takes less than 14 bit periods
            totalWriteTimeNs += 14 * MAPLE_NS_PER_BIT;
            // Multiply by the extra percentage
            totalWriteTimeNs *= (1 + (MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT / 100.0));
            // And then compute the time which the write process should complete
            mProcKillTime = time_us_64() + (totalWriteTimeNs / 1000.0 + 0.5) + 1;

            rv = true;
        }
    }

    return rv;
}

void MapleBus::processEvents(uint64_t currentTimeUs)
{
    if (isBusy())
    {
        // If currentTimeUs wasn't set, get the current time
        if (currentTimeUs == 0)
        {
            currentTimeUs = time_us_64();
        }

        if (currentTimeUs > mProcKillTime)
        {
            if (mWriteInProgress)
            {
                mSmOut.stop();
                mWriteInProgress = false;
            }
            if (mReadInProgress)
            {
                mSmIn.stop();
                mReadInProgress = false;
            }
        }
    }
}

void MapleBus::updateLastValidReadBuffer()
{
    if (mReadUpdated)
    {
        // Wait up to 1 ms for the RX FIFO to become empty (automatically drained by the read DMA)
        uint64_t timeoutTime = time_us_64() + 1000;
        while (!pio_sm_is_rx_fifo_empty(mSmIn.mProgram.mPio, mSmIn.mSmIdx)
               && time_us_64() < timeoutTime);

        mReadUpdated = false;
        uint32_t buffer[256];
        // The frame word always contains how many proceeding words there are [0,255]
        uint32_t len = mReadBuffer[0] >> 24;
        uint8_t crc = 0;
        // Bytes are loaded to the left, but the first byte is actually the LSB.
        // This means we need to byte swap (compute crc while we're at it)
        for (uint32_t i = 0; i < len + 1; ++i)
        {
            swapByteOrder(buffer[i], mReadBuffer[i], crc);
        }
        // crc in the last word does not need to be byte swapped
        // Data is only valid if the CRC is correct and first word has something in it (cmd 0 is invalid)
        if (crc == mReadBuffer[len + 1] && mReadBuffer[0] != 0)
        {
            memcpy(mLastValidRead, buffer, (len + 1) * 4);
            mLastValidReadLen = len + 1;
            mNewDataAvailable = true;
        }
    }
}

const uint32_t* MapleBus::getReadData(uint32_t& len, bool& newData)
{
    updateLastValidReadBuffer();
    if (mNewDataAvailable)
    {
        mNewDataAvailable = false;
        newData = true;
    }
    else
    {
        newData = false;
    }
    len = mLastValidReadLen;
    return mLastValidRead;
}

