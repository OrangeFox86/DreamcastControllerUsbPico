#pragma once

#include "hal/System/SystemMemory.hpp"
#include "VolatileSystemMemory.hpp"
#include "Mutex.hpp"
#include "hal/System/LockGuard.hpp"

#include "hardware/flash.h"
#include "pico/stdlib.h"

#include <assert.h>
#include <memory>
#include <string.h>
#include <list>
#include <algorithm>

// For W25Q16JV
// - Read or write of 256 byte page will take approximately 4 microseconds
// - Write can only keep a bit "1" or flip a bit from "1" to "0"
// - Erasing flips all bits to "1" within a 4096 byte sector which takes up to 400 ms

//! SystemMemory class using Pico's onboard flash
//! In order for this to work properly, program must be running from RAM
class NonVolatilePicoSystemMemory : public SystemMemory
{
public:
    //! Flash memory write states
    enum class ProgrammingState
    {
        //! Waiting for write()
        WAITING_FOR_JOB = 0,
        //! Sector erase sent, waiting for erase to complete
        SECTOR_ERASING,
        //! Delaying write to erased sector in case more writes come in for same sector
        DELAYING_WRITE,
    };

    //! Constructor
    //! @param[in] flashOffset  Offset into flash, must align to SECTOR_SIZE
    //! @param[in] size  Number of bytes to allow read/write
    NonVolatilePicoSystemMemory(uint32_t flashOffset, uint32_t size) :
        SystemMemory(),
        mOffset(flashOffset),
        mSize(size),
        mLocalMem(size),
        mMutex(),
        mProgrammingState(ProgrammingState::WAITING_FOR_JOB),
        mSectorQueue(),
        mDelayedWriteTime(0)
    {
        assert(flashOffset % SECTOR_SIZE == 0);

        // Copy all of flash into volatile memory
        const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
        mLocalMem.write(0, readFlash, size);
    }

    //! @returns number of bytes reserved in memory
    virtual uint32_t getMemorySize() final
    {
        return mSize;
    }

    //! Reads from memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in,out] size  Number of bytes to read, set with the number of bytes read
    //! @returns a pointer containing the number of bytes returned in size
    virtual const uint8_t* read(uint32_t offset, uint32_t& size) final
    {
        return mLocalMem.read(offset, size);
    }

    //! Writes to memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in] data  The data to write
    //! @param[in,out] size  Number of bytes to write, set with number of bytes written
    //! @returns true iff all bytes were written or at least queued for write
    virtual bool write(uint32_t offset, const void* data, uint32_t& size) final
    {
        LockGuard lock(mMutex, true);

        bool success = mLocalMem.write(offset, data, size);

        if (size > 0)
        {
            uint16_t firstSector = offset / SECTOR_SIZE;
            uint16_t lastSector = (offset + size - 1) / SECTOR_SIZE;
            bool delayWrite = false;
            bool itemAdded = false;
            for (uint32_t i = firstSector; i <= lastSector; ++i)
            {
                std::list<uint16_t>::iterator it =
                    std::find(mSectorQueue.begin(), mSectorQueue.end(), i);

                if (it == mSectorQueue.begin())
                {
                    if (mProgrammingState != ProgrammingState::WAITING_FOR_JOB)
                    {
                        // Currently processing this sector - delay write even further
                        delayWrite = true;
                    }
                }
                else if (it == mSectorQueue.end())
                {
                    // Add this sector
                    mSectorQueue.push_back(i);
                    itemAdded = true;
                }
            }

            if (itemAdded)
            {
                // If delaying, no longer need to delay write
                mDelayedWriteTime = 0;
            }
            else if (delayWrite)
            {
                setWriteDelay();
            }
        }

        return success;
    }

    //! Must be called to periodically process flash access
    inline void process()
    {
        mMutex.lock();
        bool unlock = true;

        switch (mProgrammingState)
        {
            case ProgrammingState::WAITING_FOR_JOB:
            {
                if (!mSectorQueue.empty())
                {
                    uint16_t sector = *mSectorQueue.begin();
                    uint32_t flashByte = sectorToFlashByte(sector);

                    setWriteDelay();
                    mProgrammingState = ProgrammingState::SECTOR_ERASING;

                    // flash_range_erase blocks until erase is complete, so don't hold the lock
                    mMutex.unlock();
                    unlock = false;
                    flash_range_erase(flashByte, SECTOR_SIZE);
                }
            }
            break;

            case ProgrammingState::SECTOR_ERASING:
            {
                // Nothing to do in this state
                mProgrammingState = ProgrammingState::DELAYING_WRITE;
            }
            // Fall through

            case ProgrammingState::DELAYING_WRITE:
            {
                if (time_us_64() >= mDelayedWriteTime)
                {
                    uint16_t sector = *mSectorQueue.begin();
                    uint32_t localByte = sector * SECTOR_SIZE;
                    uint32_t size = SECTOR_SIZE;
                    const uint8_t* mem = mLocalMem.read(localByte, size);

                    assert(mem != nullptr);

                    uint32_t flashByte = sectorToFlashByte(sector);
                    flash_range_program(flashByte, mem, SECTOR_SIZE);

                    mProgrammingState = ProgrammingState::WAITING_FOR_JOB;
                }
            }
            break;

            default:
            {
                assert(false);
            }
            break;
        }

        if (unlock)
        {
            mMutex.unlock();
        }
    }

private:
    //! Converts a local sector index to flash byte offset
    inline uint32_t sectorToFlashByte(uint16_t sector)
    {
        return mOffset + (sector * SECTOR_SIZE);
    }

    //! Set the write delay using the current time
    inline void setWriteDelay()
    {
        mDelayedWriteTime = time_us_64() + WRITE_DELAY_US;
    }

private:
    //! Number of bytes in a sector
    static const uint32_t SECTOR_SIZE = 4096;
    //! Page size in bytes
    static const uint32_t PAGE_SIZE = 256;
    //! How long to delay before committing to the last sector write
    static const uint32_t WRITE_DELAY_US = 200000;
    //! Flash memory offset
    const uint32_t mOffset;
    //! Number of bytes in volatile memory
    const uint32_t mSize;
    //! Because erase takes so long which prevents read, the entire flash range is copied locally
    VolatileSystemMemory mLocalMem;
    //! Mutex to serialize write() and flash programming
    Mutex mMutex;

    ProgrammingState mProgrammingState;
    std::list<uint16_t> mSectorQueue;
    uint64_t mDelayedWriteTime;
};
