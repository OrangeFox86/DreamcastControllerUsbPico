#include "DreamcastStorage.hpp"
#include "dreamcast_constants.h"

#include <assert.h>
#include <string.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   uint32_t fd,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, fd, scheduler, playerData.playerIndex),
    mExiting(false),
    mClock(playerData.clock),
    mUsbFileSystem(playerData.fileSystem),
    mFileName{},
    mReadState(READ_IDLE),
    mReadingTxId(0),
    mReadingBlock(-1),
    mReadPacket(nullptr)
{
    int32_t idx = subPeripheralIndex(mAddr);
    if (idx >= 0)
    {
        if (idx == 0)
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu.bin", (long unsigned int)mPlayerIndex);
        }
        else
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu-%li.bin", (long unsigned int)mPlayerIndex, (long int)idx);
        }

        mUsbFileSystem.add(this);
    }
}

DreamcastStorage::~DreamcastStorage()
{
    mExiting = true;
    // The following is externally serialized with any read() call
    mUsbFileSystem.remove(this);
}

void DreamcastStorage::task(uint64_t currentTimeUs)
{
    switch(mReadState)
    {
        case READ_STARTED:
        {
            uint32_t payload[2] = {FUNCTION_CODE, mReadingBlock};
            mReadingTxId = mEndpointTxScheduler->add(0, this, COMMAND_BLOCK_READ, payload, 2, true, 130);
            mReadState = READ_SENT;
        }
        break;

        case READ_SENT:
        {
            if (currentTimeUs >= mReadKillTime)
            {
                // Timeout
                mEndpointTxScheduler->cancelById(mReadingTxId);
                mReadState = READ_IDLE;
            }
        }
        break;

        case READ_PROCESSING:
            // Already processing, so no need to check timeout value
        default:
            break;
    }
}

void DreamcastStorage::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_IDLE && tx->transmissionId == mReadingTxId)
    {
        mReadState = READ_PROCESSING;
    }
}

void DreamcastStorage::txFailed(bool writeFailed,
                                bool readFailed,
                                std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Failure
        mReadState = READ_IDLE;
    }
}

void DreamcastStorage::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Complete!
        mReadPacket = packet;
        mReadState = READ_IDLE;
    }
}

const char* DreamcastStorage::getFileName()
{
    return mFileName;
}

uint32_t DreamcastStorage::getFileSize()
{
    return (128 * 1024);
}

int32_t DreamcastStorage::read(uint8_t blockNum,
                               void* buffer,
                               uint16_t bufferLen,
                               uint32_t timeoutUs)
{
    assert(mReadState == READ_IDLE);
    // Set data
    mReadingTxId = 0;
    mReadingBlock = blockNum;
    mReadPacket = nullptr;
    mReadKillTime = mClock.getTimeUs() + timeoutUs;
    // Commit it
    mReadState = READ_STARTED;

    // Wait for maple bus state machine to finish read
    // I'm not too happy about this blocking operation, but it works
    while(mReadState != READ_IDLE && !mExiting);

    int32_t numRead = -1;
    if (mReadPacket)
    {
        uint16_t copyLen = (bufferLen > (mReadPacket->payload.size() * 4)) ? (mReadPacket->payload.size() * 4) : bufferLen;
        // Need to flip each word before copying
        uint8_t* buffer8 = (uint8_t*)buffer;
        for (uint32_t i = 2; i < (2U + (bufferLen / 4)); ++i)
        {
            const uint32_t& originalWord = mReadPacket->payload[i];
            uint32_t flippedWord = ((originalWord << 24) & 0xFF000000)
                                    | ((originalWord << 8) & 0x00FF0000)
                                    | ((originalWord >> 8) & 0x0000FF00)
                                    | ((originalWord >> 24) & 0x000000FF);
            memcpy(buffer8, &flippedWord, 4);
            buffer8 += 4;
        }
        numRead = copyLen;
    }

    mReadPacket = nullptr;

    return numRead;
}
