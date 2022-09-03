#pragma once

#include "EndpointTxSchedulerInterface.hpp"
#include "MaplePacket.hpp"
#include "dreamcast_constants.h"
#include "PrioritizedTxScheduler.hpp"
#include <memory>

class EndpointTxScheduler : public EndpointTxSchedulerInterface
{
public:
    //! Default constructor
    EndpointTxScheduler(std::shared_ptr<PrioritizedTxScheduler> prioritizedScheduler, 
                        uint8_t fixedPriority);

    //! Virtual destructor
    virtual ~EndpointTxScheduler();

    //! Add a transmission to the schedule
    //! @param[in] txTime  Time at which this should transmit in microseconds
    //! @param[in,out] packet  Packet data to send (internal data is moved upon calling this)
    //! @param[in] expectResponse  true iff a response is expected after transmission
    //! @param[in] expectedResponseNumPayloadWords  Number of payload words to expect in response
    //! @param[in] autoRepeatUs  How often to repeat this transmission in microseconds
    //! @param[in] readTimeoutUs  Maximum amount of time to wait for full packet to be received
    //! @returns transmission ID
    virtual uint32_t add(uint64_t txTime,
                         MaplePacket& packet,
                         bool expectResponse,
                         uint32_t expectedResponseNumPayloadWords=0,
                         uint32_t autoRepeatUs=0,
                         uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US) final;

    //! Cancels scheduled transmission by transmission ID
    //! @param[in] transmissionId  The transmission ID of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelById(uint32_t transmissionId) final;

    //! Cancels scheduled transmission by recipient address
    //! @param[in] recipientAddr  The recipient address of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelByRecipient(uint8_t recipientAddr) final;

    //! Cancels all items in the schedule
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelAll() final;

private:
    std::shared_ptr<PrioritizedTxScheduler> mPrioritizedScheduler;
    uint8_t mFixedPriority;
};