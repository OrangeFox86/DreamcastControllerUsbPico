#include "TransmissionTimeliner.hpp"
#include <assert.h>

TransmissionTimeliner::TransmissionTimeliner(MapleBusInterface& bus, PrioritizedTxScheduler& schedule):
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
        assert(mNextTx->packet->isValid());
        if (mBus.write(*mNextTx->packet, mNextTx->expectResponse, mNextTx->readTimeoutUs))
        {
            pkt = mNextTx->packet;
            mNextTx = nullptr;
        }
    }

    return pkt;
}