#include "DreamcastController.hpp"
#include "dreamcast_constants.h"
#include <string.h>


DreamcastController::DreamcastController(uint8_t addr, std::shared_ptr<EndpointTxSchedulerInterface> scheduler, PlayerData playerData) :
    DreamcastPeripheral(addr, scheduler, playerData.playerIndex),
    mGamepad(playerData.gamepad),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0)
{
    mGamepad.controllerConnected();
    MaplePacket packet(COMMAND_GET_CONDITION, getRecipientAddress(), DEVICE_FN_CONTROLLER);
    mEndpointTxScheduler->add(0, packet, true, 3, US_PER_CHECK);
}

DreamcastController::~DreamcastController()
{
    mGamepad.controllerDisconnected();
}

bool DreamcastController::handleData(uint8_t len,
                                     uint8_t cmd,
                                     const uint32_t *payload)
{
    if (mWaitingForData)
    {
        mWaitingForData = false;
        mNoDataCount = 0;

        if (cmd == COMMAND_RESPONSE_DATA_XFER && len >= 3 && payload[0] == 1)
        {
            // Handle condition data
            DreamcastControllerObserver::ControllerCondition controllerCondition;
            memcpy(&controllerCondition, &payload[1], 8);
            mGamepad.setControllerCondition(controllerCondition);

            return true;
        }
    }

    return false;
}

bool DreamcastController::task(uint64_t currentTimeUs)
{
    bool connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
    if (currentTimeUs > mNextCheckTime)
    {
        // Increment count if we are still waiting for response from the last communication attempt
        if (mWaitingForData)
        {
            ++mNoDataCount;
            mWaitingForData = false;
            connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
        }

        if (connected)
        {
            mWaitingForData = true;
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
    return connected;
}
