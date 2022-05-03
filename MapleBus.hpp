#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/dma.h"
#include "configuration.h"
#include "utils.h"
#include "maple.pio.h"
#include "hardware/pio.h"

//! Handles communication over Maple Bus. This class is currently only setup to handle communication
//! from a "primary" device which initiates communication. This can easily be modified to handle
//! communication for a "secondary" device, but that is not a use-case of this project.
class MapleBus
{
    public:
        //! Enumerates all of the valid commands for Dreamcast devices
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
        //! Maple Bus constructor
        //! @param[in] pinA  GPIO index for pin A. The very next GPIO will be designated as pin B.
        //! @param[in] senderAddr  The address of this device
        MapleBus(uint32_t pinA, uint8_t senderAddr);

        //! Writes a command to the Maple Bus where the sender address is what was given in the
        //! constructor.
        //! @param[in] command  The command byte - should be a value in Command enumeration
        //! @param[in] recipientAddr  The address of the device receiving this command
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @returns true iff the bus was "open" and send has started
        bool write(uint8_t command, uint8_t recipientAddr, const uint32_t* payload, uint8_t len, bool expectResponse);

        //! Writes a command with the given custom frame word. The internal sender address is
        //! ignored and instead the given frame word is sent verbatim.
        //! @param[in] frameWord  The first word to put out on the bus
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @returns true iff the bus was "open" and send has started
        bool write(uint32_t frameWord, const uint32_t* payload, uint8_t len, bool expectResponse);

        //! Writes a command with the given words. The internal sender address is ignored and
        //! instead the given words are sent verbatim.
        //! @param[in] words  All words to send
        //! @param[in] len  Number of words in words (must be at least 1)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @returns true iff the bus was "open" and send has started
        bool write(const uint32_t* words, uint8_t len, bool expectResponse);

        //! Called from a PIO ISR when read has completed for this sender.
        void readIsr();

        //! Called from a PIO ISR when write has completed for this sender.
        void writeIsr();

        //! Retrieves the last valid set of data read.
        //! @param[out] len  The number of words received
        //! @param[out] newData  Set to true iff new data was received since the last call
        //! @returns a pointer to the bytes read.
        //! @warning this call should be serialized with calls to write() as those calls may change
        //!          the data in the underlying buffer which is returned.
        const uint32_t* getReadData(uint32_t& len, bool& newData);

    private:
        //! Ensures that the bus is open and starts the write PIO state machine.
        bool writeInit();

        //! Processes timing events for the current time.
        void processEvents();

        //! If new data is available and is valid, updates mLastValidRead.
        void updateLastValidReadBuffer();

        //! Swaps the endianness of a 32 bit word from the given source to the given destination.
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

        //! @returns the program offset for the PIO program maple_out
        static uint getOutProgramOffset();

        //! @returns the program offset for the PIO program maple_in
        static uint getInProgramOffset();

        //! Initializes all interrupt service routines for all Maple Busses
        static void initIsrs();

    public:
        //! The PIO bank used for the maple_out program
        static pio_hw_t* const PIO_OUT;
        //! The PIO bank used for the maple_in program
        static pio_hw_t* const PIO_IN;

    private:
        //! Total number of DMAs used by Maple Busses (2 used by each instance)
        static uint kDmaCount;

        //! Pin A GPIO index for this bus
        const uint32_t mPinA;
        //! Pin B GPIO index for this bus
        const uint32_t mPinB;
        //! Pin A GPIO mask for this bus
        const uint32_t mMaskA;
        //! Pin B GPIO mask for this bus
        const uint32_t mMaskB;
        //! GPIO mask for all bits used by this bus
        const uint32_t mMaskAB;
        //! The address of this device
        const uint8_t mSenderAddr;
        //! The PIO state machine index used for output by this bus
        const uint mSmOutIdx;
        //! The PIO state machine index used for input by this bus
        const uint mSmInIdx;
        //! The DMA channel used for writing by this bus
        const uint mDmaWriteChannel;
        //! The DMA channel used for reading by this bus
        const uint mDmaReadChannel;

        //! The output word buffer - 256 + 2 extra words for bit count and CRC
        uint32_t mWriteBuffer[258];
        //! The input word buffer - 256 + 1 extra word for CRC
        uint32_t mReadBuffer[257];
        //! Holds the last know valid read buffer (no CRC - that was validated)
        uint32_t mLastValidRead[256];
        //! Number of words stored in mLastValidRead, including the frame word
        uint32_t mLastValidReadLen;
        //! True iff mLastValidRead was updated since last call to getReadData()
        bool mNewDataAvailable;
        //! True iff mReadBuffer was updated since last call to updateLastValidReadBuffer()
        volatile bool mReadUpdated;
        //! True when write is currently in progress
        volatile bool mWriteInProgress;
        //! True if read should be started immediately after write has completed
        bool mExpectingResponse;
        //! True when read is currently in progress
        volatile bool mReadInProgress;
        //! The time at which the next timeout will occur
        uint64_t mProcKillTime;
};

#endif // __MAPLE_BUS_H__
