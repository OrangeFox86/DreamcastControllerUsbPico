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

bool DreamcastMainNode::handleData(uint8_t len,
                                   uint8_t cmd,
                                   const uint32_t *payload)
{
    // Handle device info from main peripheral
    if (cmd == COMMAND_RESPONSE_DEVICE_INFO)
    {
        if (len > 0)
        {
            peripheralFactory(payload[0]);
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

    return handlePeripheralData(len, cmd, payload);
}

void DreamcastMainNode::task(uint64_t currentTimeUs)
{
    TransmissionTimeliner::ReadStatus readStatus = mTransmissionTimeliner.readTask(currentTimeUs);

    // See if there is anything to receive
    if (readStatus.received != nullptr)
    {
        uint8_t sendAddr = readStatus.received->getFrameSenderAddr();
        uint8_t recAddr = readStatus.received->getFrameRecipientAddr();
        uint8_t cmd = readStatus.received->getFrameCommand();
        const uint32_t* payload = readStatus.received->payload.data();

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
                handleData(readStatus.received->payload.size(), cmd, payload);
            }
            else
            {
                int32_t idx = DreamcastPeripheral::subPeripheralIndex(sendAddr);
                if (idx >= 0 && (uint32_t)idx < mSubNodes.size())
                {
                    mSubNodes[idx]->handleData(readStatus.received->payload.size(), cmd, payload);
                }
            }
        }
    }

    // Allow peripherals and subnodes to handle time-dependent tasks
    if (mPeripherals.size() > 0)
    {
        // Have the connected main peripheral handle write
        if (handlePeripherals(currentTimeUs))
        {
            for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
                 iter != mSubNodes.end();
                 ++iter)
            {
                (*iter)->task(currentTimeUs);
            }
        }
        else
        {
            // Main peripheral disconnected
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
            for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
                 iter != mSubNodes.end();
                 ++iter)
            {
                if (recipientAddr == (*iter)->getRecipientAddress())
                {
                    (*iter)->txSent(sentTx);
                    break;
                }
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
