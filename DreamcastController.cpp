#include "DreamcastController.hpp"
#include <string.h>


DreamcastController::DreamcastController(MapleBus& bus, uint32_t playerIndex, UsbGamepad& gamepad) :
    DreamcastMainPeripheral(bus),
    mPlayerIndex(playerIndex),
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

    if (cmd == MapleBus::COMMAND_RESPONSE_DATA_XFER && len >= 3 && payload[0] == 1)
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

bool DreamcastController::task(uint64_t currentTimeUs)
{
    bool rv = true;

    if (currentTimeUs > mNextCheckTime)
    {
        // See if we need to update connection status to disconnected
        if (mWaitingForData)
        {
            if (++mNoDataCount >= NO_DATA_DISCONNECT_COUNT)
            {
                mNoDataCount = 0;
                rv = false;
            }
        }
        else
        {
            // Get controller status
            uint32_t data = 1;
            if (mBus.write(MapleBus::COMMAND_GET_CONDITION, 0x20, &data, 1, true))
            {
                mWaitingForData = true;
                mNextCheckTime = currentTimeUs + US_PER_CHECK;
            }
        }
    }
    else
    {
        yieldTask(currentTimeUs, mNextCheckTime - currentTimeUs);
    }

    return rv;
}
