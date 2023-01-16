#include "DreamcastMainPeripheral.hpp"

#include <assert.h>
#include <stdio.h>

namespace client
{

DreamcastMainPeripheral::DreamcastMainPeripheral(uint8_t addr,
                                                 uint8_t regionCode,
                                                 uint8_t connectionDirectionCode,
                                                 const char* descriptionStr,
                                                 const char* producerStr,
                                                 const char* versionStr,
                                                 float standbyCurrentmA,
                                                 float maxCurrentmA) :
    DreamcastPeripheral(addr,
                        regionCode,
                        connectionDirectionCode,
                        descriptionStr,
                        producerStr,
                        versionStr,
                        standbyCurrentmA,
                        maxCurrentmA),
    mPlayerIndex(0),
    mSubPeripherals()
{}

void DreamcastMainPeripheral::addSubPeripheral(std::shared_ptr<DreamcastPeripheral> subPeripheral)
{
    // This sub-peripheral's address must be unique
    assert(mSubPeripherals.find(subPeripheral->mAddr) == mSubPeripherals.end());
    assert(subPeripheral->mAddr != mAddr);
    // Add it
    mSubPeripherals.insert(std::pair<uint8_t, std::shared_ptr<DreamcastPeripheral>>(
        subPeripheral->mAddr, subPeripheral));
    // Accumulate to my address (main peripheral communicates back what sub peripherals are attached)
    mAddrAugmenter |= subPeripheral->mAddr;
}

bool DreamcastMainPeripheral::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    uint8_t playerIdx = (in.frame.senderAddr & PLAYER_ID_ADDR_MASK) >> PLAYER_ID_BIT_SHIFT;
    setPlayerIndex(playerIdx);

    return DreamcastPeripheral::handlePacket(in, out);
}

bool DreamcastMainPeripheral::dispensePacket(const MaplePacket& in, MaplePacket& out)
{
    uint8_t rawRecipientAddr = in.frame.recipientAddr & ~PLAYER_ID_ADDR_MASK;
    if (rawRecipientAddr == mAddr)
    {
        // This is for me
        return handlePacket(in, out);
    }
    else if (mSubPeripherals.find(rawRecipientAddr) != mSubPeripherals.end())
    {
        // This is for one of my sub-peripherals
        return mSubPeripherals[rawRecipientAddr]->handlePacket(in, out);
    }
    else
    {
        // This is invalid
        return false;
    }
}

void DreamcastMainPeripheral::reset()
{
    DreamcastPeripheral::reset();
    mPlayerIndex = 0;
    for (std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>>::iterator iter = mSubPeripherals.begin();
         iter != mSubPeripherals.end();
         ++iter)
    {
        iter->second->reset();
    }
}

void DreamcastMainPeripheral::setPlayerIndex(uint8_t idx)
{
    assert(idx < 4);
    if (!mConnected || idx != mPlayerIndex)
    {
        if (mConnected)
        {
            // About to make a connection - first reset
            reset();
        }
        // Set player index value in the augmenters
        mPlayerIndex = idx;
        uint8_t augmenterMask = (idx << PLAYER_ID_BIT_SHIFT);
        mAddrAugmenter = (mAddrAugmenter & ~PLAYER_ID_ADDR_MASK) | augmenterMask;
        for (std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>>::iterator iter = mSubPeripherals.begin();
             iter != mSubPeripherals.end();
             ++iter)
        {
            // The only augmenter in sub-peripherals is player index
            iter->second->setAddrAugmenter(augmenterMask);
        }
    }
}

}
