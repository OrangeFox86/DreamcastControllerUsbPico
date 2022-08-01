#pragma once

#include "MaplePacket.hpp"
#include "dreamcast_constants.h"
#include <list>
#include <memory>

class TransmittionScheduler
{
public:
    //! Transmission definition
    struct Transmission
    {
        const uint32_t transmissionId;
        const bool highPriority;
        const bool expectResponse;
        const uint32_t readTimeoutUs;
        const uint32_t autoRepeatUs;
        const uint32_t txDurationUs;
        uint64_t nextTxTimeUs;
        const std::shared_ptr<MaplePacket> packet;

        Transmission(uint32_t transmissionId,
                     bool highPriority,
                     bool expectResponse,
                     uint32_t readTimeoutUs,
                     uint32_t autoRepeatUs,
                     uint32_t txDurationUs,
                     uint64_t nextTxTimeUs,
                     std::shared_ptr<MaplePacket> packet):
            transmissionId(transmissionId),
            highPriority(highPriority),
            expectResponse(expectResponse),
            readTimeoutUs(readTimeoutUs),
            autoRepeatUs(autoRepeatUs),
            txDurationUs(txDurationUs),
            nextTxTimeUs(nextTxTimeUs),
            packet(packet)
        {}
    };

public:
    //! Default constructor
    TransmittionScheduler();

    //! Virtual destructor
    virtual ~TransmittionScheduler();

    //! Add a transmission to the schedule
    //! @param[in] highPritority  true iff this is a high priority transmission
    //! @param[in] txTime  Time at which this should transmit in microseconds
    //! @param[in,out] packet  Packet data to send (internal data is moved upon calling this)
    //! @param[in] expectResponse  true iff a response is expected after transmission
    //! @param[in] expectedResponseNumPayloadWords  Number of payload words to expect in response
    //! @param[in] autoRepeatUs  How often to repeat this transmission in microseconds
    //! @param[in] readTimeoutUs  Maximum amount of time to wait for full packet to be received
    //! @returns transmission ID
    uint32_t add(bool highPriority,
                 uint64_t txTime,
                 MaplePacket& packet,
                 bool expectResponse,
                 uint32_t expectedResponseNumPayloadWords=0,
                 uint32_t autoRepeatUs=0,
                 uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US);

    //! Pops the next scheduled packet, given the current time
    //! @param[in] time  The current time
    //! @returns nullptr if no scheduled packet is available for the given time
    //! @returns the next sceduled packet for the given current time
    std::shared_ptr<const Transmission> popNext(uint64_t time);

    //! Cancels scheduled transmission by transmission ID
    //! @param[in] transmissionId  The transmission ID of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    uint32_t cancelById(uint32_t transmissionId);

    //! Cancels scheduled transmission by recipient address
    //! @param[in] recipientAddr  The recipient address of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    uint32_t cancelByRecipient(uint8_t recipientAddr);

    //! Cancels all items in the schedule
    //! @returns number of transmissions successfully canceled
    uint32_t cancelAll();

protected:
    //! Add a transmission to the schedule
    //! @param[in] tx  The transmission to add
    //! @returns transmission ID
    uint32_t add(std::shared_ptr<Transmission> tx);

public:
    //! Estimated nanoseconds before peripheral responds
    static const uint32_t RX_DELAY_NS = 50;
    //! Estimated nanoseconds per bit to receive data
    static const uint32_t RX_NS_PER_BIT = 1500;
    //! Use this for txTime if the packet needs to be sent ASAP
    static const uint64_t TX_TIME_ASAP = 0;

protected:
    //! The next transmission ID to set
    uint32_t mNextId;
    //! The current schedule ordered by time and priority
    std::list<std::shared_ptr<Transmission>> mSchedule;
};