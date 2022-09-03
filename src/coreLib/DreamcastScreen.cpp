#include "DreamcastScreen.hpp"
#include "dreamcast_constants.h"

DreamcastScreen::DreamcastScreen(uint8_t addr, 
                                 std::shared_ptr<EndpointTxSchedulerInterface> scheduler, 
                                 PlayerData playerData) :
    DreamcastPeripheral(addr, scheduler, playerData.playerIndex),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0),
    mUpdateRequired(true),
    mScreenData(playerData.screenData),
    mTransmissionId(0)
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

        if (connected && (mScreenData.isNewDataAvailable() || mUpdateRequired))
        {
            // Write screen data
            static const uint8_t partitionNum = 0; // Always 0
            static const uint8_t sequenceNum = 0;  // 1 and only 1 in this sequence - always 0
            static const uint16_t blockNum = 0;    // Always 0
            static const uint32_t writeAddrWord = (partitionNum << 24) | (sequenceNum << 16) | blockNum;
            uint32_t numPayloadWords = ScreenData::NUM_SCREEN_WORDS + 2;
            uint32_t payload[numPayloadWords] = {DEVICE_FN_LCD, writeAddrWord, 0};
            mScreenData.readData(&payload[2]);

            // Workaround: make sure previous tx is canceled in case it hasn't gone out yet
            mEndpointTxScheduler->cancelById(mTransmissionId);

            MaplePacket packet(COMMAND_BLOCK_WRITE, getRecipientAddress(), payload, numPayloadWords);
            mTransmissionId = mEndpointTxScheduler->add(PrioritizedTxScheduler::TX_TIME_ASAP, packet, true, 0);
            mWaitingForData = true;
            mNextCheckTime = currentTimeUs + US_PER_CHECK;

            mUpdateRequired = false;
        }
    }
    return true;
}