#pragma once

#include "MapleBusInterface.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockedMapleBus : public MapleBusInterface
{
    public:
        MOCK_METHOD(
            bool,
            write,
            (
                uint8_t command,
                uint8_t recipientAddr,
                const uint32_t* payload,
                uint8_t len,
                bool expectResponse,
                uint32_t readTimeoutUs
            ),
            (override)
        );

        bool write(uint8_t command,
                   uint8_t recipientAddr,
                   const uint32_t* payload,
                   uint8_t len,
                   bool expectResponse)
        {
            return write(command, recipientAddr, payload, len, expectResponse, DEFAULT_MAPLE_READ_TIMEOUT_US);
        }

        MOCK_METHOD(
            bool,
            write,
            (
                uint32_t frameWord,
                const uint32_t* payload,
                uint8_t len,
                bool expectResponse,
                uint32_t readTimeoutUs
            ),
            (override)
        );

        bool write(uint32_t frameWord,
                   const uint32_t* payload,
                   uint8_t len,
                   bool expectResponse)
        {
            return write(frameWord, payload, len, expectResponse, DEFAULT_MAPLE_READ_TIMEOUT_US);
        }

        MOCK_METHOD(
            bool,
            write,
            (
                const uint32_t* words,
                uint8_t len,
                bool expectResponse,
                uint32_t readTimeoutUs
            ),
            (override)
        );

        bool write(const uint32_t* words,
                   uint8_t len,
                   bool expectResponse)
        {
            return write(words, len, expectResponse, DEFAULT_MAPLE_READ_TIMEOUT_US);
        }

        MOCK_METHOD(const uint32_t*, getReadData, (uint32_t& len, bool& newData), (override));

        MOCK_METHOD(void, processEvents, (uint64_t currentTimeUs), (override));

        void processEvents()
        {
            processEvents(0);
        }

        MOCK_METHOD(bool, isBusy, (), (override));
};
