#include "DreamcastSubNode.hpp"
#include "dreamcast_constants.h"

DreamcastSubNode::DreamcastSubNode(uint8_t addr, MapleBus& bus, uint32_t playerIndex) :
    DreamcastNode(addr),
    mBus(bus),
    mPlayerIndex(playerIndex),
    mNextCheckTime(0),
    mConnected(false)
{
}

DreamcastSubNode::DreamcastSubNode(const DreamcastSubNode& rhs) :
    DreamcastNode(rhs),
    mBus(rhs.mBus),
    mPlayerIndex(rhs.mPlayerIndex),
    mNextCheckTime(rhs.mNextCheckTime),
    mConnected(rhs.mConnected)
{
}

bool DreamcastSubNode::handleData(uint8_t len,
                        uint8_t cmd,
                        const uint32_t *payload)
{
    // If device info received, add the sub peripheral
    if (cmd == COMMAND_RESPONSE_DEVICE_INFO)
    {
        // TODO: Handle peripherals here

        return (mPeripherals.size() > 0);
    }

    // Pass data to sub peripheral
    return handlePeripheralData(len, cmd, payload);
}

void DreamcastSubNode::task(uint64_t currentTimeUs)
{
    if (mConnected && currentTimeUs >= mNextCheckTime && !mBus.isBusy())
    {
        // Request device info new device was newly attached
        if (mPeripherals.size() <= 0)
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
        // Handle operations for peripherals (run task() of all peripherals)
        else
        {
            // Have the connected main peripheral handle write
            bool disconnected = !handlePeripherals(currentTimeUs);
            if (disconnected)
            {
                // Peripheral(s) disconnected
                mNextCheckTime = currentTimeUs;
            }
        }
    }
}

void DreamcastSubNode::mainPeripheralDisconnected()
{
    mPeripherals.clear();
}

void DreamcastSubNode::setConnected(bool connected)
{
    if (mConnected != connected)
    {
        mConnected = connected;
        if (!mConnected)
        {
            // Once something has been disconnected, clear all peripherals
            mPeripherals.clear();
        }
    }
}