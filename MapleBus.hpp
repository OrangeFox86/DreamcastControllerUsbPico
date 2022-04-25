#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "configuration.h"
#include "utils.h"

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
        MapleBus(uint32_t pinA, uint32_t pinB, uint8_t senderAddr);
        bool write(uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len);
        bool read(uint32_t* words,
                  uint32_t& len,
                  uint32_t waitTimeoutUs = DEFAULT_SYSTICK_READ_WAIT_US,
                  uint32_t readTimeoutUs = DEFAULT_SYSTICK_READ_TIMEOUT_US);


    private:

        bool writeInit();
        void writeComplete();

        void writeStartSequence();
        void writeEndSequence();
        void writeByte(const uint8_t& byte);
        void writeBytes(const uint8_t* bytePtr, const uint8_t* endPtr);

        void putAB(const uint32_t& value);
        void toggleA();
        void toggleB();
        void delay();

        uint32_t mLastPut;
        const uint32_t mPinA;
        const uint32_t mPinB;
        const uint32_t mMaskA;
        const uint32_t mMaskB;
        const uint32_t mMaskAB;
        const uint8_t mSenderAddr;
        uint32_t mCrc;

        // Number of CPU clock ticks equal a clocking period
        static const uint32_t CPU_TICKS_PER_WRITE_PERIOD = (INT_DIVIDE_ROUND(MIN_CLOCK_PERIOD_NS * CPU_FREQ_MHZ, 1000));
        // 8 is approximately the number of operations after the while loop that it takes to write
        // bits and reset cvr
        static const uint32_t WRITE_CLOCK_BIT_BIAS = 8;
        // Reload value for systick
        static const uint32_t SYSTICK_WRITE_RELOAD_VALUE = (CPU_TICKS_PER_WRITE_PERIOD - 1 - WRITE_CLOCK_BIT_BIAS);
        // The actual systick period after applying bias
        static const uint32_t SYSTICK_WRITE_PERIOD_NS = ((SYSTICK_WRITE_RELOAD_VALUE + 1) * (1000.0 / CPU_FREQ_MHZ) + 0.5);
        // Number of clocking cycles we wait to confirm line is neutral before taking control
        static const uint32_t OPEN_LINE_CHECK_CYCLES = (INT_DIVIDE_CEILING(OPEN_LINE_CHECK_TIME_NS, SYSTICK_WRITE_PERIOD_NS));
};

#endif // __MAPLE_BUS_H__
