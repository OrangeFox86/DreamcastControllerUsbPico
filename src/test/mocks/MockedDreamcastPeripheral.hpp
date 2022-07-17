#pragma once

#include "DreamcastPeripheral.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockedDreamcastPeripheral : public DreamcastPeripheral
{
    public:
        MockedDreamcastPeripheral(uint8_t addr, MapleBusInterface& bus, uint32_t playerIndex) :
            DreamcastPeripheral(addr, bus, playerIndex)
        {}

        MOCK_METHOD(bool,
                    handleData,
                    (uint8_t len, uint8_t cmd, const uint32_t *payload),
                    (override));

        MOCK_METHOD(bool, task, (uint64_t currentTimeUs), (override));
};
