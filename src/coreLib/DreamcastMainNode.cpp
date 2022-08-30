#include "DreamcastMainNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"
#include "DreamcastController.hpp"
#include "EndpointTxScheduler.hpp"

DreamcastMainNode::DreamcastMainNode(MapleBusInterface& bus,
                                     PlayerData playerData,
                                     std::shared_ptr<PrioritizedTxScheduler> prioritizedTxScheduler) :
    DreamcastNode(DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK, 
                  std::make_shared<EndpointTxScheduler>(
                    prioritizedTxScheduler,
                    MAIN_TRANSMISSION_PRIORITY
                  ),
                  playerData), 
    mBus(bus),
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
    mBus.processEvents(currentTimeUs);

    // See if there is anything to receive
    uint32_t len = 0;
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

            if (sendAddr & mAddr)
            {
                // Use the sender address to determine what sub peripherals are connected
                for (std::vector<std::shared_ptr<DreamcastSubNode>>::iterator iter = mSubNodes.begin();
                     iter != mSubNodes.end();
                     ++iter)
                {
                    uint8_t mask = (*iter)->getAddr();
                    (*iter)->setConnected((sendAddr & mask) != 0);
                }

                // Have the device handle the data
                handleData(len, cmd, payload);
            }
            else
            {
                int32_t idx = DreamcastPeripheral::subPeripheralIndex(sendAddr);
                if (idx >= 0 && (uint32_t)idx < mSubNodes.size())
                {
                    mSubNodes[idx]->handleData(len, cmd, payload);
                }
            }
        }
    }

    // See if there is something that needs to write
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
            addInfoRequestToSchedule();
        }
    }

    std::shared_ptr<const MaplePacket> handledPacket = mTransmissionTimeliner.task(currentTimeUs);
    // TODO: let peripheral know this packet was sent
}

void DreamcastMainNode::addInfoRequestToSchedule()
{
    MaplePacket packet(COMMAND_DEVICE_INFO_REQUEST,
                       DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr),
                       NULL,
                       0);
    mScheduleId = mEndpointTxScheduler->add(
        PrioritizedTxScheduler::TX_TIME_ASAP,
        packet,
        true,
        EXPECTED_DEVICE_INFO_PAYLOAD_WORDS,
        US_PER_CHECK);
}
