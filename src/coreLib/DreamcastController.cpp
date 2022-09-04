#include "DreamcastController.hpp"
#include "dreamcast_constants.h"
#include <string.h>


DreamcastController::DreamcastController(uint8_t addr, std::shared_ptr<EndpointTxSchedulerInterface> scheduler, PlayerData playerData) :
    DreamcastPeripheral(addr, scheduler, playerData.playerIndex),
    mGamepad(playerData.gamepad),
    mIsConnected(true),
    mWaitingForData(false),
    mFirstTask(true),
    mConditionTxId(0)
{
    mGamepad.controllerConnected();
}

DreamcastController::~DreamcastController()
{
    mGamepad.controllerDisconnected();
}

void DreamcastController::txSent(std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
{
    if (mConditionTxId != 0 && tx->transmissionId == mConditionTxId)
    {
        mWaitingForData = true;
    }
}

void DreamcastController::txFailed(bool writeFailed,
                                   bool readFailed,
                                   std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
{
    if (mConditionTxId != 0 && tx->transmissionId == mConditionTxId)
    {
        mIsConnected = false;
        mWaitingForData = false;
    }
}

bool DreamcastController::handleData(uint8_t len,
                                     uint8_t cmd,
                                     const uint32_t *payload)
{
    if (mWaitingForData)
    {
        mWaitingForData = false;

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
    if (mFirstTask && mIsConnected)
    {
        mFirstTask = false;
        MaplePacket packet(COMMAND_GET_CONDITION, getRecipientAddress(), DEVICE_FN_CONTROLLER);
        uint64_t txTime = PrioritizedTxScheduler::computeNextTimeCadence(currentTimeUs, US_PER_CHECK);
        mConditionTxId = mEndpointTxScheduler->add(txTime, packet, true, 3, US_PER_CHECK);
    }

    return mIsConnected;
}
