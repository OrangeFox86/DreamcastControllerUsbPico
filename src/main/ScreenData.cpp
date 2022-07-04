#include "ScreenData.hpp"
#include <string.h>
#include <assert.h>

ScreenData::ScreenData() :
    mScreenData(),
    mReadStorage(),
    mNewDataAvailable(false)
{
    critical_section_init(&mCriticalSection);
}

void ScreenData::setData(uint32_t* data, uint32_t startIndex, uint32_t numWords)
{
    assert(startIndex + numWords <= sizeof(mScreenData));
    critical_section_enter_blocking(&mCriticalSection);
    memcpy(mScreenData + startIndex, data, numWords);
    mNewDataAvailable = true;
    critical_section_exit(&mCriticalSection);
}

bool ScreenData::isNewDataAvailable() const
{
    return mNewDataAvailable;
}

const uint32_t* ScreenData::readData()
{
    critical_section_enter_blocking(&mCriticalSection);
    mNewDataAvailable = false;
    memcpy(mReadStorage, mScreenData, sizeof(mReadStorage));
    critical_section_exit(&mCriticalSection);
    return mReadStorage;
}
