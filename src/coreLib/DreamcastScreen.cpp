#include "DreamcastScreen.hpp"
#include "dreamcast_constants.h"

DreamcastScreen::DreamcastScreen(uint8_t addr, MapleBusInterface& bus, PlayerData playerData) :
    DreamcastPeripheral(addr, bus, playerData.playerIndex),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0),
    mFirstWrite(true),
    mScreenData(playerData.screenData)
{}

DreamcastScreen::~DreamcastScreen()
{}

bool DreamcastScreen::handleData(uint8_t len,
                        uint8_t cmd,
                        const uint32_t *payload)
{
    if (mWaitingForData)
    {
        mWaitingForData = false;
        mNoDataCount = 0;

        // TODO: return code is ignored for now; in the future, try to resend on failure

        return true;
    }
    return false;
}

bool DreamcastScreen::task(uint64_t currentTimeUs)
{
    bool connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
    if (currentTimeUs > mNextCheckTime)
    {
        // Increment count if we are still waiting for response from the last communication attempt
        if (mWaitingForData)
        {
            ++mNoDataCount;
            mWaitingForData = false;
            connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
        }

        if (connected && (mScreenData.isNewDataAvailable() || mFirstWrite))
        {
            // Write screen data
            static const uint8_t partitionNum = 0; // Always 0
            static const uint8_t sequenceNum = 0;  // 1 and only 1 in this sequence - always 0
            static const uint16_t blockNum = 0;    // Always 0
            static const uint32_t writeAddrWord = (partitionNum << 24) | (sequenceNum << 16) | blockNum;
            uint32_t payload[ScreenData::NUM_SCREEN_WORDS + 2] = {DEVICE_FN_LCD, writeAddrWord, 0};
            mScreenData.readData(&payload[2]);

            if (mBus.write(COMMAND_BLOCK_WRITE, getRecipientAddress(), payload, sizeof(payload), true))
            {
                mWaitingForData = true;
                mNextCheckTime = currentTimeUs + US_PER_CHECK;
            }

            mFirstWrite = false;
        }
    }
    return true;
}