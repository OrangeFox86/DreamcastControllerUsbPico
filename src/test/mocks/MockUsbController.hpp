#pragma once

#include "hal/Usb/UsbControllerInterface.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockUsbController : public UsbControllerInterface
{
    public:
        MOCK_METHOD(bool, isButtonPressed, (), (override));
        MOCK_METHOD(void, updateAllReleased, (), (override));
        MOCK_METHOD(bool, send, (bool force), (override));
        bool send() { return send(false); }
        MOCK_METHOD(uint8_t, getReportSize, (), (override));
        MOCK_METHOD(void, getReport, (uint8_t *buffer, uint16_t reqlen), (override));
        MOCK_METHOD(void, updateUsbConnected, (bool connected), (override));
        MOCK_METHOD(bool, isUsbConnected, (), (override));
        MOCK_METHOD(void, updateControllerConnected, (bool connected), (override));
        MOCK_METHOD(bool, isControllerConnected, (), (override));
};
