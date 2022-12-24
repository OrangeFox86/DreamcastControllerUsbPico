#pragma once

#include <atomic>
#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"
#include "hal/Usb/UsbFile.hpp"
#include "hal/Usb/UsbFileSystem.hpp"
#include "hal/System/ClockInterface.hpp"

//! Handles communication with the Dreamcast storage peripheral
class DreamcastStorage : public DreamcastPeripheral, UsbFile
{
    public:
        //! The current state of the READ and WRITE state machines
        enum ReadWriteState : uint8_t
        {
            //! Read state machine is idle
            READ_WRITE_IDLE = 0,
            //! read() or write() has been executed, waiting for Maple Bus state machine to start r/w
            READ_WRITE_STARTED,
            //! The Maple Bus state machine has queued r/w
            READ_WRITE_SENT,
            //! The Maple Bus state machine is currently processing r/w
            READ_WRITE_PROCESSING
        };

        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this storage device
        DreamcastStorage(uint8_t addr,
                        std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                        PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastStorage();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        // The following are inherited from UsbFile

        //! @returns file name
        virtual const char* getFileName() final;

        //! @returns file size in bytes (currently only up to 128KB supported)
        virtual uint32_t getFileSize() final;

        //! Blocking read (must only be called from the core not operating maple bus)
        //! @param[in] blockNum  Block number to read (block is 512 bytes)
        //! @param[out] buffer  Buffer output
        //! @param[in] bufferLen  The length of buffer (but only up to 512 bytes will be written)
        //! @param[in] timeoutUs  Timeout in microseconds
        //! @returns Positive value indicating how many bytes were read
        //! @returns Zero if read failure occurred
        //! @returns Negative value if timeout elapsed
        virtual int32_t read(uint8_t blockNum,
                             void* buffer,
                             uint16_t bufferLen,
                             uint32_t timeoutUs) final;

        //! Blocking write (must only be called from the core not operating maple bus)
        //! @param[in] blockNum  Block number to write (block is 512 bytes)
        //! @param[in] buffer  Buffer
        //! @param[in] bufferLen  The length of buffer (but only up to 512 bytes will be written)
        //! @param[in] timeoutUs  Timeout in microseconds
        //! @returns Positive value indicating how many bytes were written
        //! @returns Zero if write failure occurred
        //! @returns Negative value if timeout elapsed
        virtual int32_t write(uint8_t blockNum,
                              const void* buffer,
                              uint16_t bufferLen,
                              uint32_t timeoutUs) final;

    private:
        //! Flips the endianness of a word
        //! @param[in] word  Input word
        //! @returns output word
        static uint32_t flipWordBytes(const uint32_t& word);

    public:
        //! Function code for storage
        static const uint32_t FUNCTION_CODE = DEVICE_FN_STORAGE;

    private:
        //! Initialized false and set to true when destructor called
        bool mExiting;
        //! Reference to a clock which allows us to keep track of time for timeout
        ClockInterface& mClock;
        //! Reference to a file system where this object may be added to
        UsbFileSystem& mUsbFileSystem;
        //! File name for this storage device
        char mFileName[12];

        //! The current state in the read state machine
        //! When READ_WRITE_IDLE: read() can read and write the data below
        //! Otherwise: peripheral callbacks can read and write the data below
        std::atomic<ReadWriteState> mReadState;

        //! Transmission ID of the read operation sent (or 0)
        uint32_t mReadingTxId;
        //! The block number of the current read operation
        uint8_t mReadingBlock;
        //! Packet filled in as a result of a read operation
        std::shared_ptr<const MaplePacket> mReadPacket;
        //! Time at which read must be killed
        uint64_t mReadKillTime;

        //! The current state in the write state machine
        //! When READ_WRITE_IDLE: write() can read and write the data below
        //! Otherwise: peripheral callbacks can read and write the data below
        std::atomic<ReadWriteState> mWriteState;

        //! Transmission ID of the write operation sent (or 0)
        uint32_t mWritingTxId;
        //! The block number of the current write operation
        uint8_t mWritingBlock;
        //! Pointer to the data to be written
        const void* mWriteBuffer;
        //! Length of write buffer
        int32_t mWriteBufferLen;
        //! Time at which write must be killed
        uint64_t mWriteKillTime;
};
