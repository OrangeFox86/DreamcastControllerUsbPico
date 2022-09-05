#include "DreamcastSubNode.hpp"
#include "dreamcast_constants.h"
#include "utils.h"

DreamcastSubNode::DreamcastSubNode(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastNode(addr, scheduler, playerData),
    mConnected(false),
    mScheduleId(-1)
{
}

DreamcastSubNode::DreamcastSubNode(const DreamcastSubNode& rhs) :
    DreamcastNode(rhs),
    mConnected(rhs.mConnected),
    mScheduleId(rhs.mScheduleId)
{
}

void DreamcastSubNode::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{
    // If device info received, add the sub peripheral
    if (packet->getFrameCommand() == COMMAND_RESPONSE_DEVICE_INFO)
    {
        if (packet->payload.size() > 0)
        {
            peripheralFactory(packet->payload[0]);
            if (mPeripherals.size() > 0)
            {
                // Remove the auto reload device info request transmission from schedule
                if (mScheduleId >= 0)
                {
                    mEndpointTxScheduler->cancelById(mScheduleId);
                    mScheduleId = -1;
                }
                DEBUG_PRINT("Player %lu sub node 0x%02hX connected\n",
                            mPlayerData.playerIndex + 1,
                            getRecipientAddress());
            }
        }
    }
}

void DreamcastSubNode::task(uint64_t currentTimeUs)
{
    if (mConnected)
    {
        // Handle operations for peripherals (run task() of all peripherals)
        if (mPeripherals.size() > 0)
        {
            // Have the connected main peripheral handle write
            handlePeripherals(currentTimeUs);
        }
    }
}

void DreamcastSubNode::mainPeripheralDisconnected()
{
    setConnected(false);
}

void DreamcastSubNode::setConnected(bool connected, uint64_t currentTimeUs)
{
    if (mConnected != connected)
    {
        mConnected = connected;
        mPeripherals.clear();
        mEndpointTxScheduler->cancelByRecipient(getRecipientAddress());
        if (mConnected)
        {
            // Keep asking for info until valid response is heard
            MaplePacket packet(
                COMMAND_DEVICE_INFO_REQUEST,
                getRecipientAddress(),
                NULL,
                0);
            uint64_t txTime = PrioritizedTxScheduler::TX_TIME_ASAP;
            if (currentTimeUs > 0)
            {
                txTime = PrioritizedTxScheduler::computeNextTimeCadence(currentTimeUs, US_PER_CHECK);
            }
            mScheduleId = mEndpointTxScheduler->add(
                txTime,
                this,
                packet,
                true,
                EXPECTED_DEVICE_INFO_PAYLOAD_WORDS,
                US_PER_CHECK);
        }
        else
        {
            DEBUG_PRINT("Player %lu sub node 0x%02hX disconnected\n",
                        mPlayerData.playerIndex + 1,
                        getRecipientAddress());
        }
    }
}