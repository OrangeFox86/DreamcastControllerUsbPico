#include "DreamcastStorage.hpp"
#include "dreamcast_constants.h"

#include <string.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, scheduler, playerData.playerIndex),
    mExiting(false),
    mClock(playerData.clock),
    mUsbFileSystem(playerData.fileSystem),
    mFileName{},
    mReadingTxId(0),
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
{}

void DreamcastStorage::txStarted(std::shared_ptr<const Transmission> tx)
{
}

void DreamcastStorage::txFailed(bool writeFailed,
                                bool readFailed,
                                std::shared_ptr<const Transmission> tx)
{
}

void DreamcastStorage::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{
    if (tx->transmissionId == mReadingTxId)
    {
        mReadPacket = packet;
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
    int32_t numRead = -1;
    uint64_t startTimeUs = mClock.getTimeUs();
    mReadPacket.reset();
    uint32_t payload[2] = {FUNCTION_CODE, blockNum};
    mReadingTxId = mEndpointTxScheduler->add(0, this, COMMAND_BLOCK_READ, payload, 2, true, 130);

    // I'm not too happy about this blocking operation, but it works
    while ((mClock.getTimeUs() - startTimeUs) < timeoutUs && !mExiting)
    {
        if (mReadPacket != nullptr)
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
            break;
        }
    }

    return numRead;
}
