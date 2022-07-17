#pragma once

#include "DreamcastControllerObserver.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockedDreamcastControllerObserver : public DreamcastControllerObserver
{
    public:
        MOCK_METHOD(void, setControllerCondition, (const ControllerCondition& controllerCondition), (override));

        MOCK_METHOD(void, controllerConnected, (), (override));

        MOCK_METHOD(void, controllerDisconnected, (), (override));
};
