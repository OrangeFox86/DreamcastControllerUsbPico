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
            return write(packet, expectResponse, DEFAULT_MAPLE_READ_TIMEOUT_US);
        }

        MOCK_METHOD(const uint32_t*, getReadData, (uint32_t& len, bool& newData), (override));

        MOCK_METHOD(void, processEvents, (uint64_t currentTimeUs), (override));

        void processEvents()
        {
            processEvents(0);
        }

        MOCK_METHOD(bool, isBusy, (), (override));
};
