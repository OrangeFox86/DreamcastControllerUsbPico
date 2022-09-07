#include "DreamcastArGun.hpp"

DreamcastArGun::DreamcastArGun(uint8_t addr,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("AR gun", addr, scheduler, playerData.playerIndex)
{}

DreamcastArGun::~DreamcastArGun()
{}

void DreamcastArGun::task(uint64_t currentTimeUs)
{}

void DreamcastArGun::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastArGun::txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx)
{}

void DreamcastArGun::txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx)
{}
