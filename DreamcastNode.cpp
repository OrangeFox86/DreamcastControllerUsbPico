#include "DreamcastNode.hpp"
#include "DreamcastController.hpp"
#include "DreamcastPeripheral.hpp"
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

    // See if there is anything to receive
    uint32_t len;
    bool newData = false;
    const uint32_t* dat = mBus.getReadData(len, newData);
    if (newData)
    {
        uint8_t sendAddr = *dat >> 8 & 0xFF;
        uint8_t recAddr = *dat >> 16 & 0xFF;
        uint8_t cmd = *dat >> 24;
        const uint32_t* payload = dat + 1;
        --len;

        if (recAddr == 0x00)
        {
            // This packet was meant for me

            if (mMainPeripheral)
            {
                // Handle data and ignore result
                (void)mMainPeripheral->handleData(len, sendAddr, cmd, payload);
            }
            else
            {
                // Handle device info from main peripheral
                if (cmd == MapleBus::COMMAND_RESPONSE_DEVICE_INFO)
                {
                    if (payload[0] & DEVICE_FN_CONTROLLER)
                    {
                        mMainPeripheral = std::make_unique<DreamcastController>(
                            mBus, mPlayerIndex, mGamepad);
                    }
                }
            }
        }
    }

    // See if there is something that needs to write
    if (mMainPeripheral)
    {
        // Have the connected main peripheral handle write
        if (!mMainPeripheral->task(currentTimeUs))
        {
            // Main peripheral disconnected
            mMainPeripheral.reset();
        }
    }
    // Otherwise, keep looking for info from a main peripheral
    else if (currentTimeUs > mNextCheckTime)
    {
        if (mBus.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST,
                       DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK,
                       NULL,
                       0,
                       true))
        {
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
}
