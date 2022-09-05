#pragma once

#include <stdint.h>
#include <memory>
#include "MaplePacket.hpp"
#include "Transmitter.hpp"

//! Transmission definition
struct Transmission
{
    //! Unique ID of this transmission
    const uint32_t transmissionId;
    //! Priority where 0 is highest
    const uint8_t priority;
    //! Set to true iff a response is expected
    const bool expectResponse;
    const uint32_t readTimeoutUs;
    const uint32_t autoRepeatUs;
    const uint32_t txDurationUs;
    uint64_t nextTxTimeUs;
    std::shared_ptr<const MaplePacket> packet;
    Transmitter* const transmitter;

    Transmission(uint32_t transmissionId,
                 uint8_t priority,
                 bool expectResponse,
                 uint32_t readTimeoutUs,
                 uint32_t autoRepeatUs,
                 uint32_t txDurationUs,
                 uint64_t nextTxTimeUs,
                 std::shared_ptr<MaplePacket> packet,
                 Transmitter* transmitter):
        transmissionId(transmissionId),
        priority(priority),
        expectResponse(expectResponse),
        readTimeoutUs(readTimeoutUs),
        autoRepeatUs(autoRepeatUs),
        txDurationUs(txDurationUs),
        nextTxTimeUs(nextTxTimeUs),
        packet(packet),
        transmitter(transmitter)
    {}

    //! @returns the estimated completion time of this transmission
    uint64_t getNextCompletionTime()
    {
        return nextTxTimeUs + txDurationUs;
    }
};