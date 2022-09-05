#pragma once

#include "MapleBusInterface.hpp"
#include "MaplePacket.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockMapleBus : public MapleBusInterface
{
    public:
        MOCK_METHOD(
            bool,
            write,
            (
                const MaplePacket& packet,
                bool expectResponse,
                uint32_t readTimeoutUs
            ),
            (override)
        );

        bool write(const MaplePacket& packet, bool expectResponse)
        {
            return write(packet, expectResponse, 0);
        }

        MOCK_METHOD(Status, processEvents, (uint64_t currentTimeUs), (override));

        MOCK_METHOD(bool, isBusy, (), (override));
};
