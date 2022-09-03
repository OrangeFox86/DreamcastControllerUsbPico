#include "TransmissionTimeliner.hpp"
#include <assert.h>

TransmissionTimeliner::TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule):
    mBus(bus), mSchedule(schedule), mNextTx(nullptr)
{}

std::shared_ptr<const MaplePacket> TransmissionTimeliner::task(uint64_t time)
{
    std::shared_ptr<const MaplePacket> pkt = nullptr;

    if (mNextTx == nullptr && !mBus.isBusy())
    {
        mNextTx = mSchedule->popNext(time);
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