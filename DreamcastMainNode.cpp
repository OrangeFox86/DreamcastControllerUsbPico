#include "DreamcastMainNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"
#include "DreamcastController.hpp"

DreamcastMainNode::DreamcastMainNode(uint32_t mapleBusPinA, PlayerData playerData) :
    DreamcastNode(DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK, playerData),
    mBus(mapleBusPinA, 0x00),
    mNextCheckTime(0),
    mSubNodes{DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(0), mBus, mPlayerData),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(1), mBus, mPlayerData),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(2), mBus, mPlayerData),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(3), mBus, mPlayerData),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(4), mBus, mPlayerData)}
{

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
        if (payload[0] & DEVICE_FN_CONTROLLER)
        {
            mPeripherals.clear();
            mPeripherals.push_back(std::unique_ptr<DreamcastController>(new DreamcastController(mAddr, mBus, mPlayerData)));
        }
        // TODO: Handle other peripherals here

        return (mPeripherals.size() > 0);
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
                for (int32_t i = 0; i < DreamcastPeripheral::MAX_SUB_PERIPHERALS; ++i)
                {
                    uint8_t mask = mSubNodes[i].getAddr();
                    mSubNodes[i].setConnected((sendAddr & mask) != 0);
                }

                // Have the device handle the data
                handleData(len, cmd, payload);
            }
            else
            {
                int32_t idx = DreamcastPeripheral::subPeripheralIndex(sendAddr);
                if (idx >= 0 && idx < DreamcastPeripheral::MAX_SUB_PERIPHERALS)
                {
                    mSubNodes[idx].handleData(len, cmd, payload);
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
            for (uint32_t i = 0; i < NUM_SUB_NODES; ++i)
            {
                mSubNodes[i].task(currentTimeUs);
            }
        }
        else
        {
            // Main peripheral disconnected
            for (uint32_t i = 0; i < NUM_SUB_NODES; ++i)
            {
                mSubNodes[i].mainPeripheralDisconnected();
            }
            mNextCheckTime = currentTimeUs;
        }
    }
    // Otherwise, keep looking for info from a main peripheral
    else if (currentTimeUs >= mNextCheckTime)
    {
        if (mBus.write(COMMAND_DEVICE_INFO_REQUEST,
                       DreamcastPeripheral::getRecipientAddress(
                           mPlayerData.playerIndex,
                           DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK),
                       NULL,
                       0,
                       true))
        {
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
}