#include "DreamcastNode.hpp"
#include "DreamcastController.hpp"
#include <string.h>

// Device functions
#define DEVICE_FN_CONTROLLER    0x00000001
#define DEVICE_FN_STORAGE       0x00000002
#define DEVICE_FN_LCD           0x00000004
#define DEVICE_FN_TIMER         0x00000008
#define DEVICE_FN_AUDIO_INPUT   0x00000010
#define DEVICE_FN_AR_GUN        0x00000020
#define DEVICE_FN_KEYBOARD      0x00000040
#define DEVICE_FN_GUN           0x00000080
#define DEVICE_FN_VIBRATION     0x00000100
#define DEVICE_FN_MOUSE         0x00000200
#define DEVICE_FN_EXMEDIA       0x00000400
#define DEVICE_FN_CAMERA        0x00000800

DreamcastNode::DreamcastNode(uint32_t mapleBusPinA, uint32_t playerIndex, UsbGamepad& gamepad) :
    mBus(mapleBusPinA, 0x00),
    mPlayerIndex(playerIndex),
    mGamepad(gamepad),
    mNextCheckTime(0)
{

}

void DreamcastNode::task(uint64_t currentTimeUs)
{
    mBus.processEvents(currentTimeUs);

    if (mMainPeripheral)
    {
        if (!mMainPeripheral->task(currentTimeUs))
        {
            mMainPeripheral.reset();
        }
    }
    else
    {
        uint32_t len;
        bool newData = false;
        const uint32_t* dat = mBus.getReadData(len, newData);

        if (newData)
        {
            uint8_t cmd = *dat >> 24;
            if (cmd == MapleBus::COMMAND_RESPONSE_DEVICE_INFO)
            {
                if (dat[1] & DEVICE_FN_CONTROLLER)
                {
                    mMainPeripheral = std::make_unique<DreamcastController>(
                        mBus, mPlayerIndex, mGamepad);
                }
            }
        }

        if (currentTimeUs > mNextCheckTime)
        {
            if (mBus.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x20, NULL, 0, true))
            {
                mNextCheckTime = currentTimeUs + US_PER_CHECK;
            }
        }
    }
}
