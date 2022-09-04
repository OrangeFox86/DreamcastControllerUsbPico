#include "TransmissionTimeliner.hpp"
#include <assert.h>

TransmissionTimeliner::TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule):
    mBus(bus), mSchedule(schedule), mNextTx(nullptr)
{}

TransmissionTimeliner::ReadStatus TransmissionTimeliner::readTask(uint64_t currentTimeUs)
{
    ReadStatus status;

    // Process bus events and get any data received
    MapleBusInterface::Status busStatus = mBus.processEvents(currentTimeUs);
    status.lastRxFailed = busStatus.readFail;
    status.lastTxFailed = busStatus.writeFail;
    if (busStatus.readBuffer != nullptr)
    {
        status.received = std::make_shared<MaplePacket>(busStatus.readBuffer,
                                                        busStatus.readBufferLen);
    }

    return status;
}

std::shared_ptr<const PrioritizedTxScheduler::Transmission> TransmissionTimeliner::writeTask(uint64_t currentTimeUs)
{
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> txSent = nullptr;

    // Get next transmission
    if (mNextTx == nullptr && !mBus.isBusy())
    {
        mNextTx = mSchedule->popNext(currentTimeUs);
    }

    // Transmit
    if (mNextTx != nullptr)
    {
        assert(mNextTx->packet->isValid());
        if (mBus.write(*mNextTx->packet, mNextTx->expectResponse, mNextTx->readTimeoutUs))
        {
            txSent = mNextTx;
            mNextTx = nullptr;
        }
    }

    return txSent;
}