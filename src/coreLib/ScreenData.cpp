#include "ScreenData.hpp"
#include <string.h>
#include <assert.h>
#include <mutex>

ScreenData::ScreenData(MutexInterface& mutex) :
    mMutex(mutex),
    mScreenData(),
    mReadStorage(),
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

const uint32_t* ScreenData::readData()
{
    std::lock_guard<MutexInterface> lockGuard(mMutex);
    mNewDataAvailable = false;
    memcpy(mReadStorage, mScreenData, sizeof(mReadStorage));
    return mReadStorage;
}
