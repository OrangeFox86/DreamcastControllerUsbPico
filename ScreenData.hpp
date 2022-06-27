#pragma once

#include "pico/critical_section.h"
#include <stdint.h>

class ScreenData
{
    public:


    public:
        ScreenData();

        void setData(uint32_t* data, uint32_t startIndex=0, uint32_t numWords=NUM_SCREEN_WORDS);

        bool isNewDataAvailable() const;

        const uint32_t* readData();

    public:
        static const uint32_t NUM_SCREEN_WORDS = 48;

    private:
        critical_section_t mCriticalSection;
        uint32_t mScreenData[NUM_SCREEN_WORDS];
        uint32_t mReadStorage[NUM_SCREEN_WORDS];
        bool mNewDataAvailable;
};
