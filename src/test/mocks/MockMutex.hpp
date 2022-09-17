#pragma once

#include "hal/System/MutexInterface.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockMutex : public MutexInterface
{
    public:
        MOCK_METHOD(void, lock, (), (override));

        MOCK_METHOD(void, unlock, (), (override));
};
