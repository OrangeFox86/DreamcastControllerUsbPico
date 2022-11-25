#pragma once

#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"
#include "hal/Usb/UsbFile.hpp"
#include "hal/Usb/UsbFileSystem.hpp"
#include "hal/System/ClockInterface.hpp"

//! Handles communication with the Dreamcast storage peripheral
class DreamcastStorage : public DreamcastPeripheral, UsbFile
{
    public:
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
        //! The transmission ID of the current read operation
        uint32_t mReadingTxId;
        //! Packet filled in as a result of a read operation
        std::shared_ptr<const MaplePacket> mReadPacket;
};