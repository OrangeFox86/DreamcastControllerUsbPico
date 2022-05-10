#ifndef __DREAMCAST_NODE_H__
#define __DREAMCAST_NODE_H__

#include "MapleBus.hpp"
#include "UsbGamepad.h"

class DreamcastNode
{
    public:
        struct ControllerCondition
        {
            union
            {
                uint32_t words[2];
                // For digital bits:
                // 0: pressed
                // 1: released
                struct
                {
                    uint8_t l; // 255: fully pressed

                    uint8_t r; // 255: fully pressed

                    unsigned none2:1;
                    unsigned y:1;
                    unsigned x:1;
                    unsigned none1:5;

                    unsigned none3:1;
                    unsigned b:1;
                    unsigned a:1;
                    unsigned start:1;
                    unsigned up:1;
                    unsigned down:1;
                    unsigned left:1;
                    unsigned right:1;

                    uint8_t rAnalogUD; // Always 128

                    uint8_t rAnalogLR; // Always 128

                    uint8_t lAnalogUD; // 0: up; 128: neutral; 255: down

                    uint8_t lAnalogLR; // 0: left; 128: neutral; 255: right
                };
            };

            ControllerCondition()
            {
                reset();
            }

            void reset()
            {
                words[0] = 0xFFFF0000;
                words[1] = 0x80808080;
            }
        };
    public:
        DreamcastNode(uint32_t mapleBusPinA, uint32_t playerIndex, UsbGamepad& gamepad);

        void task(uint64_t currentTimeUs);

        bool isAPressed()
        {
            return (0 == mControllerCondition.a);
        }

    private:
        static const uint32_t MAX_NUM_PLAYERS = 4;
        static const uint32_t US_PER_CHECK = 16000;
        static const uint32_t NO_DATA_DISCONNECT_COUNT = 5;
        MapleBus mBus;
        const uint32_t mPlayerIndex;
        UsbGamepad& mGamepad;
        bool mControllerDetected;
        uint64_t mNextCheckTime;
        bool mWaitingForData;
        uint32_t mNoDataCount;
        ControllerCondition mControllerCondition;
};

#endif // __DREAMCAST_NODE_H__
