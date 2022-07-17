#pragma once

#include "MutexInterface.hpp"
#include <stdint.h>

class ScreenData
{
    public:
        ScreenData(MutexInterface& mutex);

        void setData(uint32_t* data, uint32_t startIndex=0, uint32_t numWords=NUM_SCREEN_WORDS);

        bool isNewDataAvailable() const;

        void readData(uint32_t* out);

    public:
        static const uint32_t NUM_SCREEN_WORDS = 48;

    private:
        MutexInterface& mMutex;
        uint32_t mScreenData[NUM_SCREEN_WORDS];
        bool mNewDataAvailable;
};
