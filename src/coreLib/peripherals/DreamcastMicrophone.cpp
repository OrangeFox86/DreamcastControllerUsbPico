#include "DreamcastMicrophone.hpp"

DreamcastMicrophone::DreamcastMicrophone(uint8_t addr,
                                         std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                         PlayerData playerData) :
    DreamcastPeripheral("microphone", addr, scheduler, playerData.playerIndex)
{}

DreamcastMicrophone::~DreamcastMicrophone()
{}

void DreamcastMicrophone::task(uint64_t currentTimeUs)
{}

void DreamcastMicrophone::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastMicrophone::txFailed(bool writeFailed,
                                   bool readFailed,
                                   std::shared_ptr<const Transmission> tx)
{}

void DreamcastMicrophone::txComplete(std::shared_ptr<const MaplePacket> packet,
                                     std::shared_ptr<const Transmission> tx)
{}
