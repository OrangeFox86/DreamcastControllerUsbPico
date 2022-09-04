#include "PrioritizedTxScheduler.hpp"
#include "utils.h"

// STL
#include <algorithm>

PrioritizedTxScheduler::PrioritizedTxScheduler(): mNextId(1), mSchedule() {}

PrioritizedTxScheduler::~PrioritizedTxScheduler() {}

uint32_t PrioritizedTxScheduler::add(std::shared_ptr<Transmission> tx)
{
    // Keep iterating until correct position is found
    // For this to be the right position, it either needs to start and end before the next
    // packet or starts before the next packet and is higher or same priority.
    std::list<std::shared_ptr<Transmission>>::const_iterator iter = mSchedule.cbegin();
    while(iter != mSchedule.cend())
    {
        // If this transmission starts before this element...
        if (tx->nextTxTimeUs < (*iter)->nextTxTimeUs)
        {
            // if this transmission also ends before this element starts
            // or is of higher or same priority...
            if (tx->getNextCompletionTime() < (*iter)->nextTxTimeUs
                || tx->priority <= (*iter)->priority)
            {
                // Stop here
                break;
            }
        }
        // If this transmission starts before this element completes
        // and is higher priority...
        else if (tx->nextTxTimeUs < (*iter)->getNextCompletionTime()
                    && tx->priority < (*iter)->priority)
        {
            // Stop here
            break;
        }
        ++iter;
    }

    mSchedule.insert(iter, tx);
    return tx->transmissionId;
}

uint32_t PrioritizedTxScheduler::add(uint8_t priority,
                                    uint64_t txTime,
                                    MaplePacket& packet,
                                    bool expectResponse,
                                    uint32_t expectedResponseNumPayloadWords,
                                    uint32_t autoRepeatUs,
                                    uint32_t readTimeoutUs)
{
    uint32_t pktDurationNs = MAPLE_OPEN_LINE_CHECK_TIME_US + packet.getTxTimeNs();

    if (expectResponse)
    {
        pktDurationNs +=
            RX_DELAY_NS + MaplePacket::getTxTimeNs(expectedResponseNumPayloadWords, RX_NS_PER_BIT);
    }

    uint32_t pktDurationUs = INT_DIVIDE_CEILING(pktDurationNs, 1000);

    std::shared_ptr<Transmission> tx =
        std::make_shared<Transmission>(mNextId++,
                                       priority,
                                       expectResponse,
                                       readTimeoutUs,
                                       autoRepeatUs,
                                       pktDurationUs,
                                       txTime,
                                       false,
                                       std::make_shared<MaplePacket>(std::move(packet)));

    return add(tx);
}

uint32_t PrioritizedTxScheduler::addReset(uint8_t priority, uint64_t txTime, uint32_t autoRepeatUs)
{
    std::shared_ptr<Transmission> tx =
        std::make_shared<Transmission>(mNextId++,
                                       priority,
                                       false,
                                       DEFAULT_MAPLE_READ_TIMEOUT_US,
                                       autoRepeatUs,
                                       RESET_SEND_DURATION_US,
                                       txTime,
                                       true,
                                       nullptr);

    return add(tx);
}

uint64_t PrioritizedTxScheduler::computeNextTimeCadence(uint64_t currentTime,
                                                        uint64_t period,
                                                        uint64_t offset)
{
    // Cover the edge case where the offset is in the future for some reason
    if (offset > currentTime)
    {
        return offset;
    }
    else
    {
        // Determine how many intervals to advance past offset
        uint32_t n = INT_DIVIDE_CEILING(currentTime - offset, period);
        if (n == 0)
        {
            n = 1;
        }
        // Compute the next cadence
        return offset + (period * n);
    }
}

std::shared_ptr<const PrioritizedTxScheduler::Transmission> PrioritizedTxScheduler::popNext(uint64_t time)
{
    std::shared_ptr<Transmission> item = nullptr;

    if (!mSchedule.empty())
    {
        if (time >= (*mSchedule.begin())->nextTxTimeUs)
        {
            item = (*mSchedule.begin());
            mSchedule.erase(mSchedule.begin());
        }
        else
        {
            // See if a item down the schedule could start now and end before mSchedule.begin().
            // It's easier to check here than to rearrange schedule every time a higher priority
            // transmission preempts a lower priority one.
            std::list<std::shared_ptr<Transmission>>::iterator iter = mSchedule.begin();
            std::list<uint8_t> recipientAddrs;
            uint8_t recipientAddr = 0;
            if ((*iter)->packet != nullptr)
            {
                recipientAddr = (*iter)->packet->getFrameRecipientAddr();
            }
            recipientAddrs.push_back(recipientAddr);
            ++iter;
            while(iter != mSchedule.end())
            {
                uint8_t recipientAddr = 0;
                if ((*iter)->packet != nullptr)
                {
                    recipientAddr = (*iter)->packet->getFrameRecipientAddr();
                }
                if (time >= (*iter)->nextTxTimeUs
                    && (*iter)->getNextCompletionTime() < (*mSchedule.begin())->nextTxTimeUs
                    // Ensure the order is preserved for each recipient address
                    && std::find(recipientAddrs.begin(),
                                 recipientAddrs.end(),
                                 recipientAddr) == recipientAddrs.end())
                {
                    item = (*iter);
                    mSchedule.erase(iter);
                    break;
                }
                recipientAddrs.push_back(recipientAddr);
                ++iter;
            }
        }

        if (item != nullptr && item->autoRepeatUs > 0)
        {
            item->nextTxTimeUs = computeNextTimeCadence(time, item->autoRepeatUs, item->nextTxTimeUs);
            add(item);
        }
    }

    return item;
}

uint32_t PrioritizedTxScheduler::cancelById(uint32_t transmissionId)
{
    uint32_t n = 0;
    std::list<std::shared_ptr<Transmission>>::iterator iter = mSchedule.begin();
    while (iter != mSchedule.end())
    {
        if ((*iter)->transmissionId == transmissionId)
        {
            iter = mSchedule.erase(iter);
            ++n;
        }
        else
        {
            ++iter;
        }
    }
    return n;
}

uint32_t PrioritizedTxScheduler::cancelByRecipient(uint8_t recipientAddr)
{
    uint32_t n = 0;
    std::list<std::shared_ptr<Transmission>>::iterator iter = mSchedule.begin();
    while (iter != mSchedule.end())
    {
        if ((*iter)->packet != nullptr && (*iter)->packet->getFrameRecipientAddr() == recipientAddr)
        {
            iter = mSchedule.erase(iter);
            ++n;
        }
        else
        {
            ++iter;
        }
    }
    return n;
}

uint32_t PrioritizedTxScheduler::cancelAll()
{
    uint32_t n = mSchedule.size();
    mSchedule.clear();
    return n;
}
