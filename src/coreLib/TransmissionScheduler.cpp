#include "TransmissionScheduler.hpp"
#include "utils.h"

void TransmittionScheduler::add(std::shared_ptr<Transmission> tx)
{
    uint64_t completionTime = tx->nextTxTimeUs + tx->txDurationUs;

    std::list<std::shared_ptr<Transmission>>::const_iterator iter = mSchedule.cend();
    if (iter != mSchedule.cbegin())
    {
        std::list<std::shared_ptr<Transmission>>::const_iterator iterNext = iter;
        --iterNext;
        // Keep iterating until correct position is found
        // For this to be the right position, it either needs to start and end before the next
        // packet or starts before the next packet and is higher priority.
        while(iter != mSchedule.cbegin()
              && ((*iterNext)->nextTxTimeUs > completionTime
                || (
                    (*iterNext)->nextTxTimeUs + (*iterNext)->txDurationUs >= tx->nextTxTimeUs
                    && !(*iterNext)->highPriority
                    && (tx->highPriority || tx->nextTxTimeUs < (*iterNext)->nextTxTimeUs)
                )
              )
            )
        {
            --iter;
            --iterNext;
        }
    }
    mSchedule.insert(iter, tx);
}

void TransmittionScheduler::add(bool highPriority,
                                uint64_t txTime,
                                MaplePacket& packet,
                                bool expectResponse,
                                uint32_t expectedResponseNumPayloadWords,
                                uint32_t autoRepeatUs,
                                uint32_t readTimeoutUs)
{
    uint32_t pktDurationUs =
        INT_DIVIDE_CEILING(
            packet.getTxTimeNs()
                + RX_DELAY_NS
                + MaplePacket::getTxTimeNs(expectedResponseNumPayloadWords, RX_NS_PER_BIT),
            1000);
    std::shared_ptr<Transmission> tx =
        std::make_shared<Transmission>(highPriority,
                                       expectResponse,
                                       readTimeoutUs,
                                       autoRepeatUs,
                                       pktDurationUs,
                                       txTime,
                                       std::make_shared<MaplePacket>(std::move(packet)));
    add(tx);
}

std::shared_ptr<const TransmittionScheduler::Transmission> TransmittionScheduler::popNext(uint64_t time)
{
    std::shared_ptr<Transmission> item = nullptr;
    if (!mSchedule.empty() && time >= (*mSchedule.begin())->nextTxTimeUs)
    {
        item = (*mSchedule.begin());
        mSchedule.erase(mSchedule.begin());
        if (item->autoRepeatUs > 0)
        {
            // Determine how many intervals to add in cast this auto reload packet has overflowed
            uint32_t n = INT_DIVIDE_CEILING(time - item->nextTxTimeUs, item->autoRepeatUs);
            if (n == 0)
            {
                n = 1;
            }
            // Reinsert it back into the schedule with a new time
            item->nextTxTimeUs += (item->autoRepeatUs * n);
            add(item);
        }
    }
    return item;
}