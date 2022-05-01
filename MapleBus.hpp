#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/dma.h"
#include "configuration.h"
#include "utils.h"
#include "maple.pio.h"
#include "hardware/pio.h"

class MapleBus
{
    public:
        enum Command
        {
            COMMAND_DEVICE_INFO_REQUEST = 0x01,
            COMMAND_EXT_DEVICE_INFO_REQUEST = 0x02,
            COMMAND_RESET = 0x03,
            COMMAND_SHUTDOWN = 0x04,
            COMMAND_RESPONSE_DEVICE_INFO = 0x05,
            COMMAND_RESPONSE_EXT_DEVICE_INFO = 0x06,
            COMMAND_RESPONSE_ACK = 0x07,
            COMMAND_RESPONSE_DATA_XFER = 0x08,
            COMMAND_GET_CONDITION = 0x09,
            COMMAND_GET_MEMORY_INFORMATION = 0x0A,
            COMMAND_BLOCK_READ = 0x0B,
            COMMAND_BLOCK_WRITE = 0x0C,
            COMMAND_SET_CONDITION = 0x0E,
            COMMAND_RESPONSE_NONE = 0xFF,
            COMMAND_RESPONSE_FUNCTION_CODE_NOT_SUPPORTED = 0xFE,
            COMMAND_RESPONSE_UNKNOWN_COMMAND = 0xFD,
            COMMAND_RESPONSE_REQUEST_RESEND = 0xFC,
            COMMAND_RESPONSE_FILE_ERROR = 0xFB
        };

    public:
        MapleBus(uint32_t pinA, uint8_t senderAddr);
        bool write(uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len, bool expectResponse);
        bool write(uint32_t frameWord, uint32_t* words, uint8_t len, bool expectResponse);
        bool write(uint32_t* words, uint8_t len, bool expectResponse);

        void task();

        void readIsr();
        void writeIsr();

    private:
        bool writeInit();
        void killRead();
        void killWrite();
        static inline void swapByteOrder(uint32_t& dest, uint32_t source, uint8_t& crc)
        {
            const uint8_t* src = reinterpret_cast<uint8_t*>(&source);
            uint8_t* dst = reinterpret_cast<uint8_t*>(&dest) + 3;
            for (uint j = 0; j < 4; ++j, ++src, --dst)
            {
                *dst = *src;
                crc ^= *src;
            }
        }
        static uint getOutProgramOffset();
        static uint getInProgramOffset();
        static void initIsrs();

    public:
        static pio_hw_t* const PIO_OUT;
        static pio_hw_t* const PIO_IN;

    private:
        static uint kDmaCount;

        const uint32_t mPinA;
        const uint32_t mPinB;
        const uint32_t mMaskA;
        const uint32_t mMaskB;
        const uint32_t mMaskAB;
        const uint8_t mSenderAddr;
        const uint mSmOutIdx;
        const uint mSmInIdx;
        const uint mDmaWriteChannel;
        const uint mDmaReadChannel;

        uint32_t mWriteBuffer[259];
        uint32_t mReadBuffer[259];
        bool mWriteInProgress;
        bool mExpectingResponse;
        bool mReadInProgress;
        uint64_t mProcKillTime;
};

#endif // __MAPLE_BUS_H__
