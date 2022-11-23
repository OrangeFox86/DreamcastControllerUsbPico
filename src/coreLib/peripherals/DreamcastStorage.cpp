#include "DreamcastStorage.hpp"

#include <string.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, scheduler, playerData.playerIndex),
    mFileName{}
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
{}

void DreamcastStorage::txFailed(bool writeFailed,
                                bool readFailed,
                                std::shared_ptr<const Transmission> tx)
{}

void DreamcastStorage::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{}

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
    // TODO
    return 0;
}
