#include "VolatileSystemMemory.hpp"
#include <string.h>

#include "pico/stdlib.h"

VolatileSystemMemory::VolatileSystemMemory(uint32_t size) :
    SystemMemory(),
    mSize(size),
    mMemory(new uint8_t[mSize])
{
    memset(mMemory.get(), 0xFF, mSize);
}

uint32_t VolatileSystemMemory::getMemorySize()
{
    return mSize;
}

const uint8_t* VolatileSystemMemory::read(uint32_t offset, uint32_t& size)
{
    mLastActivityTime = time_us_64();

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

bool VolatileSystemMemory::write(uint32_t offset, const void* data, uint32_t& size)
{
    mLastActivityTime = time_us_64();

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

uint64_t VolatileSystemMemory::getLastActivityTime()
{
    // WARNING: Not an atomic read, but this isn't a critical thing anyway
    return mLastActivityTime;
}
