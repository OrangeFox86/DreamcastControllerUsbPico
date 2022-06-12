#include "DreamcastController.hpp"
#include "dreamcast_constants.h"
#include <string.h>


DreamcastController::DreamcastController(uint8_t addr, MapleBus& bus, uint32_t playerIndex, UsbGamepad& gamepad) :
    DreamcastPeripheral(addr, bus, playerIndex),
    mGamepad(gamepad),
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
    mWaitingForData = false;
    mNoDataCount = 0;

    if (cmd == COMMAND_RESPONSE_DATA_XFER && len >= 3 && payload[0] == 1)
    {
        // Handle condition data
        ControllerCondition controllerCondition;
        memcpy(&controllerCondition, &payload[1], 8);

        // TODO: Move magic numbers to constants
        mGamepad.setButton(0, 0 == controllerCondition.a);
        mGamepad.setButton(1, 0 == controllerCondition.b);
        mGamepad.setButton(3, 0 == controllerCondition.x);
        mGamepad.setButton(4, 0 == controllerCondition.y);
        mGamepad.setButton(11, 0 == controllerCondition.start);

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

    return false;
}

uint32_t DreamcastController::processEvents(uint64_t currentTimeUs)
{
    if (currentTimeUs > mNextCheckTime)
    {
        // Increment count if we are still waiting for response from the last communication attempt
        if (mWaitingForData)
        {
            ++mNoDataCount;
            mWaitingForData = false;
        }
    }

    return mNoDataCount;
}

void DreamcastController::task(uint64_t currentTimeUs)
{
    if (currentTimeUs > mNextCheckTime)
    {
        // Get controller status
        uint32_t data = 1; // TODO: move magic number: 1 gets button & analog stick states
        if (mBus.write(COMMAND_GET_CONDITION, getRecipientAddress(), &data, 1, true))
        {
            mWaitingForData = true;
            mNextCheckTime = currentTimeUs + US_PER_CHECK;
        }
    }
}
