#include "ScreenData.hpp"
#include <string.h>
#include <assert.h>
#include <mutex>

ScreenData::ScreenData(MutexInterface& mutex) :
    mMutex(mutex),
    // Default screen is a little VMU icon
    mScreenData{
        0x0000FFFF, 0x00000003, 0x8001C000, 0x00060000, 0x6000000C, 0x00003000, 0x0008000C, 0x10000009,
        0xDC0C1000, 0x0009DC3F, 0x10000009, 0xDC3F1000, 0x0008000C, 0x10000008, 0x360C1000, 0x00083600,
        0x10000008, 0x00001000, 0x00080000, 0x10000008, 0x7FFE1000, 0x0008FFFF, 0x10000008, 0xFFFF1000,
        0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008,
        0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF,
        0x10000008, 0x7FFE1000, 0x000C0000, 0x30000006, 0x00006000, 0x00038001, 0xC0000000, 0xFFFF0000
    },
    mNewDataAvailable(false)
{
}

void ScreenData::setData(uint32_t* data, uint32_t startIndex, uint32_t numWords)
{
    assert(startIndex + numWords <= sizeof(mScreenData));
    std::lock_guard<MutexInterface> lockGuard(mMutex);
    memcpy(mScreenData + startIndex, data, numWords);
    mNewDataAvailable = true;
}

bool ScreenData::isNewDataAvailable() const
{
    return mNewDataAvailable;
}

void ScreenData::readData(uint32_t* out)
{
    std::lock_guard<MutexInterface> lockGuard(mMutex);
    mNewDataAvailable = false;
    memcpy(out, mScreenData, sizeof(mScreenData));
}
