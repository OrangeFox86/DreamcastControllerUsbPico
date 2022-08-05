#include "TransmissionTimeliner.hpp"
#include <assert.h>

TransmissionTimeliner::TransmissionTimeliner(MapleBusInterface& bus, TransmissionScheduler& schedule):
    mBus(bus), mSchedule(schedule), mNextTx(nullptr)
{}

uint32_t TransmissionTimeliner::recipientDisconnect(uint8_t recipientAddr)
{
    return mSchedule.cancelByRecipient(recipientAddr);
}

std::shared_ptr<const MaplePacket> TransmissionTimeliner::task(uint64_t time)
{
    std::shared_ptr<const MaplePacket> pkt = nullptr;

    if (mNextTx == nullptr)
    {
        mNextTx = mSchedule.popNext(time);
    }

    if (mNextTx != nullptr)
    {
        pkt = mNextTx->packet;
        assert(pkt->isValid());
        if (mBus.write(*pkt, mNextTx->expectResponse, mNextTx->readTimeoutUs))
        {
            mNextTx = nullptr;
        }
    }

    return pkt;
}