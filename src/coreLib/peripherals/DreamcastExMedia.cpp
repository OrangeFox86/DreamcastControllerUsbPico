#include "DreamcastExMedia.hpp"

DreamcastExMedia::DreamcastExMedia(uint8_t addr,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("exmedia", addr, scheduler, playerData.playerIndex)
{}

DreamcastExMedia::~DreamcastExMedia()
{}

void DreamcastExMedia::task(uint64_t currentTimeUs)
{}

void DreamcastExMedia::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastExMedia::txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx)
{}

void DreamcastExMedia::txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx)
{}
