#pragma once

#include "MaplePacket.hpp"
#include "dreamcast_constants.h"
#include <list>
#include <memory>

class TransmittionScheduler
{
public:
    struct Transmission
    {
        const bool highPriority;
        const bool expectResponse;
        const uint32_t readTimeoutUs;
        const uint32_t autoRepeatUs;
        const uint32_t txDurationUs;
        uint64_t nextTxTimeUs;
        const std::shared_ptr<MaplePacket> packet;

        Transmission(bool highPriority,
                     bool expectResponse,
                     uint32_t readTimeoutUs,
                     uint32_t autoRepeatUs,
                     uint32_t txDurationUs,
                     uint64_t nextTxTimeUs,
                     std::shared_ptr<MaplePacket> packet):
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
    TransmittionScheduler() {}

    virtual ~TransmittionScheduler() {}

    void add(std::shared_ptr<Transmission> tx);

    //! packet is moved upon calling this
    void add(bool highPriority,
             uint64_t txTime,
             MaplePacket& packet,
             bool expectResponse,
             uint32_t expectedResponseNumPayloadWords=0,
             uint32_t autoRepeatUs=0,
             uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US);

    const std::shared_ptr<Transmission> popNext(uint64_t time);

public:
    //! Estimated nanoseconds before peripheral responds
    static const uint32_t RX_DELAY_NS = 50;
    //! Estimated nanoseconds per bit to receive data
    static const uint32_t RX_NS_PER_BIT = 1500;
    //! Use this for txTime if the packet needs to be sent ASAP
    static const uint64_t TX_TIME_ASAP = 0;
protected:
    std::list<std::shared_ptr<Transmission>> mSchedule;
};