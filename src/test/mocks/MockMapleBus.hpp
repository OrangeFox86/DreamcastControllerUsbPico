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
                bool expectResponse
            ),
            (override)
        );

        MOCK_METHOD(Status, processEvents, (uint64_t currentTimeUs), (override));

        MOCK_METHOD(bool, isBusy, (), (override));
};
