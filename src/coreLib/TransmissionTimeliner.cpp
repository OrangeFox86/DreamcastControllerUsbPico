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
        if (mNextTx->packet != nullptr)
        {
            assert(mNextTx->packet->isValid());
            assert(!mNextTx->reset);
            if (mBus.write(*mNextTx->packet, mNextTx->expectResponse, mNextTx->readTimeoutUs))
            {
                pkt = mNextTx->packet;
                mNextTx = nullptr;
            }
        }
        else
        {
            assert(mNextTx->reset);
            if (mBus.writeReset())
            {
                mNextTx = nullptr;
            }
        }
    }

    return pkt;
}