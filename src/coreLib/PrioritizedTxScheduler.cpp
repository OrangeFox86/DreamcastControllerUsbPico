#include "PrioritizedTxScheduler.hpp"
#include "configuration.h"
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
                                    Transmitter* transmitter,
                                    MaplePacket& packet,
                                    bool expectResponse,
                                    uint32_t expectedResponseNumPayloadWords,
                                    uint32_t autoRepeatUs,
                                    uint64_t autoRepeatEndTimeUs)
{
    uint32_t pktDurationNs = MAPLE_OPEN_LINE_CHECK_TIME_US + packet.getTxTimeNs();

    if (expectResponse)
    {
        uint32_t expectedReadDurationUs = MaplePacket::getTxTimeNs(expectedResponseNumPayloadWords, MAPLE_RESPONSE_NS_PER_BIT);
        pktDurationNs += MAPLE_RESPONSE_DELAY_NS + expectedReadDurationUs;
    }

    uint32_t pktDurationUs = INT_DIVIDE_CEILING(pktDurationNs, 1000);

    std::shared_ptr<Transmission> tx =
        std::make_shared<Transmission>(mNextId++,
                                       priority,
                                       expectResponse,
                                       pktDurationUs,
                                       autoRepeatUs,
                                       autoRepeatEndTimeUs,
                                       txTime,
                                       std::make_shared<MaplePacket>(std::move(packet)),
                                       transmitter);

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

std::shared_ptr<const Transmission> PrioritizedTxScheduler::popNext(uint64_t time)
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
            recipientAddrs.push_back((*iter)->packet->getFrameRecipientAddr());
            ++iter;
            while(iter != mSchedule.end())
            {
                if (time >= (*iter)->nextTxTimeUs
                    && (*iter)->getNextCompletionTime() < (*mSchedule.begin())->nextTxTimeUs
                    // Ensure the order is preserved for each recipient address
                    && std::find(recipientAddrs.begin(),
                                 recipientAddrs.end(),
                                 (*iter)->packet->getFrameRecipientAddr()) == recipientAddrs.end())
                {
                    item = (*iter);
                    mSchedule.erase(iter);
                    break;
                }
                recipientAddrs.push_back((*iter)->packet->getFrameRecipientAddr());
                ++iter;
            }
        }

        if (item != nullptr
            && item->autoRepeatUs > 0
            && (item->autoRepeatEndTimeUs == 0 || time <= item->autoRepeatEndTimeUs))
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
        if ((*iter)->packet->getFrameRecipientAddr() == recipientAddr)
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

uint32_t PrioritizedTxScheduler::countRecipients(uint8_t recipientAddr)
{
    uint32_t n = 0;

    for (std::list<std::shared_ptr<Transmission>>::iterator iter = mSchedule.begin();
         iter != mSchedule.end();
         ++iter)
    {
        if ((*iter)->packet->getFrameRecipientAddr() == recipientAddr)
        {
            ++n;
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
