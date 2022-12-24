#include "DreamcastTimer.hpp"

DreamcastTimer::DreamcastTimer(uint8_t addr,
                               uint32_t fd,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("timer", addr, fd, scheduler, playerData.playerIndex)
{}

DreamcastTimer::~DreamcastTimer()
{}

void DreamcastTimer::task(uint64_t currentTimeUs)
{}

void DreamcastTimer::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastTimer::txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx)
{}

void DreamcastTimer::txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx)
{}
