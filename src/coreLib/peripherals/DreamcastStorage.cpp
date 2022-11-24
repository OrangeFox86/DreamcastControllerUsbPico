#include "DreamcastStorage.hpp"
#include "dreamcast_constants.h"

#include "hal/System/timing.hpp"

#include <string.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, scheduler, playerData.playerIndex),
    mFileName{},
    mReadingTxId(0),
    mReadPacket(nullptr)
{
    int32_t idx = subPeripheralIndex(mAddr);
    if (idx >= 0)
    {
        if (idx == 0)
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu.bin", mPlayerIndex);
        }
        else
        {
            snprintf(mFileName, sizeof(mFileName), "vmu%lu-%li.bin", mPlayerIndex, idx);
        }

        usb_msc_add(this);
    }
}

DreamcastStorage::~DreamcastStorage()
{
    usb_msc_remove(this);
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
    uint64_t startTimeUs = get_time_us();
    mReadPacket.reset();
    uint32_t payload[2] = {FUNCTION_CODE, blockNum};
    mReadingTxId = mEndpointTxScheduler->add(0, this, COMMAND_BLOCK_READ, payload, 2, true, 130);

    while (get_time_us() - startTimeUs < timeoutUs)
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
            return copyLen;
        }
    }

    // timeout
    return -1;
}
