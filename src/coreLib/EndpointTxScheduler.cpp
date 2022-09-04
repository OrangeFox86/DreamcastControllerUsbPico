#include "EndpointTxScheduler.hpp"

EndpointTxScheduler::EndpointTxScheduler(
    std::shared_ptr<PrioritizedTxScheduler> prioritizedScheduler,
    uint8_t fixedPriority):
        mPrioritizedScheduler(prioritizedScheduler),
        mFixedPriority(fixedPriority)
{}

EndpointTxScheduler::~EndpointTxScheduler()
{}

uint32_t EndpointTxScheduler::add(uint64_t txTime,
                                  MaplePacket& packet,
                                  bool expectResponse,
                                  uint32_t expectedResponseNumPayloadWords,
                                  uint32_t autoRepeatUs,
                                  uint32_t readTimeoutUs)
{
    return mPrioritizedScheduler->add(mFixedPriority,
                                      txTime,
                                      packet,
                                      expectResponse,
                                      expectedResponseNumPayloadWords,
                                      autoRepeatUs,
                                      readTimeoutUs);
}

uint32_t EndpointTxScheduler::cancelById(uint32_t transmissionId)
{
    return mPrioritizedScheduler->cancelById(transmissionId);
}

uint32_t EndpointTxScheduler::cancelByRecipient(uint8_t recipientAddr)
{
    return mPrioritizedScheduler->cancelByRecipient(recipientAddr);
}

uint32_t EndpointTxScheduler::countRecipients(uint8_t recipientAddr)
{
    return mPrioritizedScheduler->countRecipients(recipientAddr);
}

uint32_t EndpointTxScheduler::cancelAll()
{
    return mPrioritizedScheduler->cancelAll();
}