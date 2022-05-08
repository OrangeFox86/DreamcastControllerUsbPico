#include "DreamcastNode.hpp"
#include <string.h>

DreamcastNode::DreamcastNode(uint32_t mapleBusPinA, uint32_t playerIndex) :
    mBus(mapleBusPinA, 0x00),
    mPlayerIndex(playerIndex),
    mControllerDetected(false),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0),
    mControllerCondition()
{

}

void DreamcastNode::task(uint64_t currentTimeUs)
{
    mBus.processEvents(currentTimeUs);

    uint32_t len;
    bool newData = false;
    const uint32_t* dat = mBus.getReadData(len, newData);
    if (newData)
    {
        // Process data
        mWaitingForData = false;
        mNoDataCount = 0;
        uint8_t cmd = *dat >> 24;
        if (!mControllerDetected)
        {
            // TODO: should check if actual controller device responded
            (void)dat;
            mControllerDetected = true;
            mControllerCondition.reset();
        }
        else if (cmd == MapleBus::COMMAND_RESPONSE_DATA_XFER && len >= 4 && dat[1] == 1)
        {
            // Handle condition data
            memcpy(mControllerCondition.words, &dat[2], 8);
        }
    }

    if (currentTimeUs > mNextCheckTime)
    {
        // See if we need to update connection status to disconnected
        if (mWaitingForData && mControllerDetected)
        {
            if (++mNoDataCount >= NO_DATA_DISCONNECT_COUNT)
            {
                mControllerDetected = false;
                mNoDataCount = 0;
                mControllerCondition.reset();
            }
        }

        bool writeStatus = false;
        if (!mControllerDetected)
        {
            // Most devices wait for an info request before responding to any other command
            writeStatus = mBus.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x20, NULL, 0, true);
        }
        else
        {
            // Get controller status
            uint32_t data = 1;
            writeStatus = mBus.write(MapleBus::COMMAND_GET_CONDITION, 0x20, &data, 1, true);
        }

        if (writeStatus)
        {
            mWaitingForData = true;
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
}
