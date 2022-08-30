#pragma once

#include "MaplePacket.hpp"
#include "MapleBusInterface.hpp"
#include "PrioritizedTxScheduler.hpp"

class TransmissionTimeliner
{

public:
    TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule);

    uint32_t recipientDisconnect(uint8_t recipientAddr);

    std::shared_ptr<const MaplePacket> task(uint64_t time);

protected:
    MapleBusInterface& mBus;
    std::shared_ptr<PrioritizedTxScheduler> mSchedule;
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> mNextTx;
};