#include "DreamcastMouse.hpp"

DreamcastMouse::DreamcastMouse(uint8_t addr,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("mouse", addr, scheduler, playerData.playerIndex)
{}

DreamcastMouse::~DreamcastMouse()
{}

void DreamcastMouse::task(uint64_t currentTimeUs)
{}

void DreamcastMouse::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastMouse::txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx)
{}

void DreamcastMouse::txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx)
{}
