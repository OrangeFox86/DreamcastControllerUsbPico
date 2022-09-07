#include "DreamcastStorage.hpp"

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, scheduler, playerData.playerIndex)
{}

DreamcastStorage::~DreamcastStorage()
{}

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
