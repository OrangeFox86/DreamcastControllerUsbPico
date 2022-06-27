#include "DreamcastController.hpp"
#include "dreamcast_constants.h"
#include <string.h>


DreamcastController::DreamcastController(uint8_t addr, MapleBus& bus, PlayerData playerData) :
    DreamcastPeripheral(addr, bus, playerData.playerIndex),
    mGamepad(playerData.gamepad),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0)
{
    mGamepad.updateControllerConnected(true);
}

DreamcastController::~DreamcastController()
{
    mGamepad.updateControllerConnected(false);
}

bool DreamcastController::handleData(uint8_t len,
                                     uint8_t cmd,
                                     const uint32_t *payload)
{
    if (mWaitingForData)
    {
        mWaitingForData = false;
        mNoDataCount = 0;

        if (cmd == COMMAND_RESPONSE_DATA_XFER && len >= 3 && payload[0] == 1)
        {
            // Handle condition data
            ControllerCondition controllerCondition;
            memcpy(&controllerCondition, &payload[1], 8);

            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_A, 0 == controllerCondition.a);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_B, 0 == controllerCondition.b);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_C, 0 == controllerCondition.c);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_X, 0 == controllerCondition.x);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_Y, 0 == controllerCondition.y);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_Z, 0 == controllerCondition.z);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_START, 0 == controllerCondition.start);

            // Mapping these to random unique buttons just in case something out there uses them
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_TL, 0 == controllerCondition.unknown1);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_TR, 0 == controllerCondition.unknown2);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_TL2, 0 == controllerCondition.unknown3);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_TR2, 0 == controllerCondition.unknown4);
            mGamepad.setButton(UsbGamepad::GAMEPAD_BUTTON_SELECT, 0 == controllerCondition.unknown5);

            mGamepad.setDigitalPad(UsbGamepad::DPAD_UP, 0 == controllerCondition.up);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_DOWN, 0 == controllerCondition.down);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_LEFT, 0 == controllerCondition.left);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_RIGHT, 0 == controllerCondition.right);

            mGamepad.setAnalogTrigger(true, static_cast<int32_t>(controllerCondition.l) - 128);
            mGamepad.setAnalogTrigger(false, static_cast<int32_t>(controllerCondition.r) - 128);

            mGamepad.setAnalogThumbX(true, static_cast<int32_t>(controllerCondition.lAnalogLR) - 128);
            mGamepad.setAnalogThumbY(true, static_cast<int32_t>(controllerCondition.lAnalogUD) - 128);
            mGamepad.setAnalogThumbX(false, static_cast<int32_t>(controllerCondition.rAnalogLR) - 128);
            mGamepad.setAnalogThumbY(false, static_cast<int32_t>(controllerCondition.rAnalogUD) - 128);

            mGamepad.send();

            return true;
        }
    }

    return false;
}

bool DreamcastController::task(uint64_t currentTimeUs)
{
    bool connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
    if (currentTimeUs > mNextCheckTime)
    {
        // Increment count if we are still waiting for response from the last communication attempt
        if (mWaitingForData)
        {
            ++mNoDataCount;
            mWaitingForData = false;
            connected = (mNoDataCount < NO_DATA_DISCONNECT_COUNT);
        }

        if (connected)
        {
            // Get controller status
            uint32_t data = DEVICE_FN_CONTROLLER;
            if (mBus.write(COMMAND_GET_CONDITION, getRecipientAddress(), &data, 1, true))
            {
                mWaitingForData = true;
                mNextCheckTime = currentTimeUs + US_PER_CHECK;
            }
        }
    }
    return connected;
}
