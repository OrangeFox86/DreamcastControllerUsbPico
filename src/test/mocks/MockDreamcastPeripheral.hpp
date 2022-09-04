#pragma once

#include "DreamcastPeripheral.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockDreamcastPeripheral : public DreamcastPeripheral
{
    public:
        MockDreamcastPeripheral(uint8_t addr,
                                std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                uint32_t playerIndex) :
            DreamcastPeripheral(addr, scheduler, playerIndex)
        {}

        MOCK_METHOD(bool,
                    handleData,
                    (std::shared_ptr<const MaplePacket> packet,
                        std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx),
                    (override));

        MOCK_METHOD(bool, task, (uint64_t currentTimeUs), (override));
};
