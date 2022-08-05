#pragma once

#include "MaplePacket.hpp"
#include "MapleBusInterface.hpp"
#include "TransmissionScheduler.hpp"

class TransmissionTimeliner
{

public:
    TransmissionTimeliner(MapleBusInterface& bus, TransmissionScheduler& schedule);

    uint32_t recipientDisconnect(uint8_t recipientAddr);

    std::shared_ptr<const MaplePacket> task(uint64_t time);

protected:
    MapleBusInterface& mBus;
    TransmissionScheduler& mSchedule;
    std::shared_ptr<const TransmissionScheduler::Transmission> mNextTx;
};