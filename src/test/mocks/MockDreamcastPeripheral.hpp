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
                    (uint8_t len, uint8_t cmd, const uint32_t *payload),
                    (override));

        MOCK_METHOD(bool, task, (uint64_t currentTimeUs), (override));
};
