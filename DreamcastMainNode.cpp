#include "DreamcastMainNode.hpp"
#include "DreamcastPeripheral.hpp"
#include "dreamcast_constants.h"
#include "DreamcastController.hpp"

DreamcastMainNode::DreamcastMainNode(uint32_t mapleBusPinA, uint32_t playerIndex, UsbGamepad& gamepad) :
    DreamcastNode(DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK),
    mBus(mapleBusPinA, 0x00),
    mPlayerIndex(playerIndex),
    mGamepad(gamepad),
    mNextCheckTime(0),
    mSubNodes{DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(0), mBus, playerIndex),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(1), mBus, playerIndex),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(2), mBus, playerIndex),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(3), mBus, playerIndex),
              DreamcastSubNode(DreamcastPeripheral::subPeripheralMask(4), mBus, playerIndex)}
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
            mPeripherals.push_back(std::unique_ptr<DreamcastController>(new DreamcastController(mAddr, mBus, mPlayerIndex, mGamepad)));
        }

        return (mPeripherals.size() > 0);
    }

    if (mPeripherals.size() > 0)
    {
        return mPeripherals[0]->handleData(len, cmd, payload);
    }

    return false;
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
        bool disconnected = (mPeripherals[0]->processEvents(currentTimeUs) >= NUM_FAIL_COM_DISCONNECT);
        if (disconnected)
        {
            // Main peripheral disconnected
            mPeripherals.clear();
            for (uint32_t i = 0; i < NUM_SUB_NODES; ++i)
            {
                mSubNodes[i].mainPeripheralDisconnected();
            }
            mNextCheckTime = currentTimeUs;
        }
        else
        {
            mPeripherals[0]->task(currentTimeUs);
            for (uint32_t i = 0; i < NUM_SUB_NODES; ++i)
            {
                mSubNodes[i].task(currentTimeUs);
            }
        }
    }
    // Otherwise, keep looking for info from a main peripheral
    else if (currentTimeUs >= mNextCheckTime)
    {
        if (mBus.write(COMMAND_DEVICE_INFO_REQUEST,
                       DreamcastPeripheral::getRecipientAddress(
                           mPlayerIndex, DreamcastPeripheral::MAIN_PERIPHERAL_ADDR_MASK),
                       NULL,
                       0,
                       true))
        {
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
}