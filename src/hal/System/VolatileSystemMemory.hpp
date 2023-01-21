#pragma once

#include "hal/System/SystemMemory.hpp"

#include <memory>
#include <string.h>

class VolatileSystemMemory : public SystemMemory
{
public:
    //! Constructor
    //! @param[in] size  Number of bytes to reserve in volatile memory
    VolatileSystemMemory(uint32_t size) :
        SystemMemory(),
        mSize(size),
        mMemory(new uint8_t[mSize])
    {
        memset(mMemory.get(), 0xFF, mSize);
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
        const uint8_t* mem = nullptr;
        if (offset >= mSize)
        {
            size = 0;
        }
        else
        {
            mem = &mMemory[offset];
            uint32_t max = mSize - offset;
            size = (max >= size) ? size : max;
        }
        return mem;
    }

    //! Writes to memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in] data  The data to write
    //! @param[in,out] size  Number of bytes to write, set with number of bytes written
    //! @returns true iff all bytes were written or at least queued for write
    virtual bool write(uint32_t offset, const void* data, uint32_t& size) final
    {
        bool success = false;
        uint8_t* mem = nullptr;
        if (offset >= mSize)
        {
            size = 0;
        }
        else
        {
            mem = &mMemory[offset];
            uint32_t max = mSize - offset;
            if (max >= size)
            {
                success = true;
            }
            else
            {
                size = max;
            }
            memcpy(mem, data, size);
        }
        return success;
    }

private:
    //! Number of bytes in volatile memory
    const uint32_t mSize;
    //! The volatile memory
    std::shared_ptr<uint8_t[]> mMemory;
};