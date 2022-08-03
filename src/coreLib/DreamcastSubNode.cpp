#include "DreamcastSubNode.hpp"
#include "dreamcast_constants.h"


DreamcastSubNode::DreamcastSubNode(uint8_t addr, MapleBusInterface& bus, PlayerData playerData) :
    DreamcastNode(addr, bus, playerData),
    mNextCheckTime(0),
    mConnected(false)
{
}

DreamcastSubNode::DreamcastSubNode(const DreamcastSubNode& rhs) :
    DreamcastNode(rhs),
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
        if (len > 0)
        {
            peripheralFactory(payload[0]);
            return (mPeripherals.size() > 0);
        }
        else
        {
            return false;
        }
    }

    // Pass data to sub peripheral
    return handlePeripheralData(len, cmd, payload);
}

void DreamcastSubNode::task(uint64_t currentTimeUs)
{
    if (mConnected)
    {
        // Request device info new device was newly attached
        if (mPeripherals.size() <= 0)
        {
            if (currentTimeUs >= mNextCheckTime)
            {
                MaplePacket packet(
                    COMMAND_DEVICE_INFO_REQUEST,
                    DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr),
                    NULL,
                    0);
                // This will return false if bus is busy
                if (mBus.write(packet, true))
                {
                    mNextCheckTime = currentTimeUs + US_PER_CHECK;
                }
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