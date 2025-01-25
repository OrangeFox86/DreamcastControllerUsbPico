// MIT License
//
// Copyright (c) 2022-2025 James Smith of OrangeFox86
// https://github.com/OrangeFox86/DreamcastControllerUsbPico
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ScreenData.hpp"
#include <string.h>
#include <assert.h>
#include "hal/System/LockGuard.hpp"
#include "utils.h"

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
    LockGuard lockGuard(mMutex);
    if (lockGuard.isLocked())
    {
        memcpy(mScreenData + startIndex, data, numWords);
        mNewDataAvailable = true;
    }
    else
    {
        DEBUG_PRINT("FAULT: failed to set screen data\n");
    }
}

bool ScreenData::isNewDataAvailable() const
{
    return mNewDataAvailable;
}

void ScreenData::readData(uint32_t* out)
{
    LockGuard lockGuard(mMutex);
    if (lockGuard.isLocked())
    {
        mNewDataAvailable = false;
    }
    else
    {
        DEBUG_PRINT("FAULT: failed to properly read screen data\n");
    }
    // Allow this to happen, even if locking failed
    memcpy(out, mScreenData, sizeof(mScreenData));
}
