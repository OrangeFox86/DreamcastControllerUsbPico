#ifndef __DREAMCAST_NODE_H__
#define __DREAMCAST_NODE_H__

#include "MapleBus.hpp"
#include "UsbGamepad.h"
#include "DreamcastMainPeripheral.hpp"

#include <memory>

class DreamcastNode
{
    public:
    public:
        DreamcastNode(uint32_t mapleBusPinA, uint32_t playerIndex, UsbGamepad& gamepad);

        void task(uint64_t currentTimeUs);

    private:
        static const uint32_t MAX_NUM_PLAYERS = 4;
        static const uint32_t US_PER_CHECK = 16000;
        MapleBus mBus;
        const uint32_t mPlayerIndex;
        UsbGamepad& mGamepad;
        uint64_t mNextCheckTime;
        std::unique_ptr<DreamcastMainPeripheral> mMainPeripheral;
};

#endif // __DREAMCAST_NODE_H__
