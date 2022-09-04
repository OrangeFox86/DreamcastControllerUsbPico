#include "DreamcastMainNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"
#include "DreamcastController.hpp"
#include "EndpointTxScheduler.hpp"

const uint8_t DreamcastMainNode::MAIN_TRANSMISSION_PRIORITY = 0;
const uint8_t DreamcastMainNode::SUB_TRANSMISSION_PRIORITY = 1;

DreamcastMainNode::DreamcastMainNode(MapleBusInterface& bus,
                                     PlayerData playerData,
                                     std::shared_ptr<PrioritizedTxScheduler> prioritizedTxScheduler) :
    DreamcastNode(DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK,
                  std::make_shared<EndpointTxScheduler>(
                    prioritizedTxScheduler,
                    MAIN_TRANSMISSION_PRIORITY
                  ),
                  playerData),
    mSubNodes(),
    mTransmissionTimeliner(bus, prioritizedTxScheduler),
    mScheduleId(-1)
{
    addInfoRequestToSchedule();
    mSubNodes.reserve(DreamcastPeripheral::MAX_SUB_PERIPHERALS);
    for (uint32_t i = 0; i < DreamcastPeripheral::MAX_SUB_PERIPHERALS; ++i)
    {
        mSubNodes.push_back(std::make_shared<DreamcastSubNode>(
            DreamcastPeripheral::subPeripheralMask(i),
            std::make_shared<EndpointTxScheduler>(prioritizedTxScheduler, SUB_TRANSMISSION_PRIORITY),
            mPlayerData));
    }
}

DreamcastMainNode::~DreamcastMainNode()
{}

bool DreamcastMainNode::handleData(std::shared_ptr<const MaplePacket> packet,
                                   std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
{
    // Handle device info from main peripheral
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
                DEBUG_PRINT("Player %lu main peripheral connected\n", mPlayerData.playerIndex + 1);
            }
            return (mPeripherals.size() > 0);
        }
        else
        {
            return false;
        }
    }

    return handlePeripheralData(packet, tx);
}


void DreamcastMainNode::disconnectMainPeripheral(uint64_t currentTimeUs)
{
    mPeripherals.clear();
    mEndpointTxScheduler->cancelByRecipient(getRecipientAddress());
    for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
            iter != mSubNodes.end();
            ++iter)
    {
        (*iter)->mainPeripheralDisconnected();
    }
    addInfoRequestToSchedule(currentTimeUs);
    DEBUG_PRINT("Player %lu main peripheral disconnected\n", mPlayerData.playerIndex + 1);
}

void DreamcastMainNode::task(uint64_t currentTimeUs)
{
    TransmissionTimeliner::ReadStatus readStatus = mTransmissionTimeliner.readTask(currentTimeUs);

    // See if there is anything to receive
    if (readStatus.received != nullptr)
    {
        uint8_t sendAddr = readStatus.received->getFrameSenderAddr();
        uint8_t recAddr = readStatus.received->getFrameRecipientAddr();

        if (recAddr == 0x00)
        {
            // This packet was meant for me

            if (sendAddr & mAddr)
            {
                // Use the sender address to determine what sub peripherals are connected
                for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
                     iter != mSubNodes.end();
                     ++iter)
                {
                    uint8_t mask = (*iter)->getAddr();
                    (*iter)->setConnected((sendAddr & mask) != 0, currentTimeUs);
                }

                // Have the device handle the data
                handleData(readStatus.received, readStatus.transmission);
            }
            else
            {
                int32_t idx = DreamcastPeripheral::subPeripheralIndex(sendAddr);
                if (idx >= 0 && (uint32_t)idx < mSubNodes.size())
                {
                    mSubNodes[idx]->handleData(readStatus.received, readStatus.transmission);
                }
            }
        }
    }
    else if (readStatus.lastTxFailed || readStatus.lastRxFailed)
    {
        // Let peripheral know this packet was sent
        uint8_t recipientAddr = readStatus.transmission->packet->getFrameRecipientAddr();
        if (recipientAddr == getRecipientAddress())
        {
            // A transmission failure on a main node must cause peripheral disconnect
            if (mPeripherals.size() > 0)
            {
                disconnectMainPeripheral(currentTimeUs);
            }
        }
        else
        {
            int32_t idx = DreamcastPeripheral::subPeripheralIndex(
                readStatus.transmission->packet->getFrameRecipientAddr());

            if (idx >= 0 && (uint32_t)idx < mSubNodes.size())
            {
                mSubNodes[idx]->txFailed(readStatus.lastTxFailed,
                                         readStatus.lastRxFailed,
                                         readStatus.transmission);
            }
        }
    }

    // Allow peripherals and subnodes to handle time-dependent tasks
    if (mPeripherals.size() > 0)
    {
        // Have the connected main peripheral and sub nodes handle their tasks
        handlePeripherals(currentTimeUs);
        for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
             iter != mSubNodes.end();
             ++iter)
        {
            (*iter)->task(currentTimeUs);
        }
        // The main node MUST have a recurring transmission in order to test for heartbeat
        if (mEndpointTxScheduler->countRecipients(getRecipientAddress()) == 0)
        {
            uint64_t txTime = PrioritizedTxScheduler::computeNextTimeCadence(currentTimeUs, US_PER_CHECK);
            MaplePacket packet(COMMAND_DEVICE_INFO_REQUEST,
                               DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr),
                               NULL,
                               0);
            mEndpointTxScheduler->add(txTime, packet, true, EXPECTED_DEVICE_INFO_PAYLOAD_WORDS);
        }
    }

    // Handle transmission
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> sentTx =
        mTransmissionTimeliner.writeTask(currentTimeUs);

    if (sentTx != nullptr && sentTx->packet != nullptr)
    {
        // Let peripheral know this packet was sent
        uint8_t recipientAddr = sentTx->packet->getFrameRecipientAddr();
        if (recipientAddr == getRecipientAddress())
        {
            txSent(sentTx);
        }
        else
        {
            int32_t idx = DreamcastPeripheral::subPeripheralIndex(sentTx->packet->getFrameRecipientAddr());
            if (idx >= 0 && (uint32_t)idx < mSubNodes.size())
            {
                mSubNodes[idx]->txSent(sentTx);
            }
        }
    }
}

void DreamcastMainNode::addInfoRequestToSchedule(uint64_t currentTimeUs)
{
    uint64_t txTime = PrioritizedTxScheduler::TX_TIME_ASAP;
    if (currentTimeUs > 0)
    {
        txTime = PrioritizedTxScheduler::computeNextTimeCadence(currentTimeUs, US_PER_CHECK);
    }
    MaplePacket packet(COMMAND_DEVICE_INFO_REQUEST,
                       DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr),
                       NULL,
                       0);
    mScheduleId = mEndpointTxScheduler->add(
        txTime,
        packet,
        true,
        EXPECTED_DEVICE_INFO_PAYLOAD_WORDS,
        US_PER_CHECK);
}
