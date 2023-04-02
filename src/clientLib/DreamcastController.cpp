#include "DreamcastController.hpp"
#include <string.h>

namespace client
{

DreamcastController::DreamcastController() :
    DreamcastController({
        .enLeftD = true,
        .enRightD = false,
        .enLeftA = true,
        .enRightA = true,
        .enL = true,
        .enR = true,
        .enStart = true,
        .enA = true,
        .enB = true,
        .enC = true,
        .enD = true,
        .enX = true,
        .enY = true,
        .enZ = true
    })
{}

DreamcastController::DreamcastController(EnabledControls enabledControls) :
    DreamcastPeripheralFunction(DEVICE_FN_CONTROLLER),
    mEnabledControls(enabledControls),
    mConditionSamples(0)
{
    setCondition(NEUTRAL_CONTROLLER_CONDITION);
}

bool DreamcastController::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    const uint8_t cmd = in.frame.command;
    if (cmd == COMMAND_GET_CONDITION)
    {
        ++mConditionSamples;
        out.frame.command = COMMAND_RESPONSE_DATA_XFER;
        out.reservePayload(3);
        out.appendPayload(getFunctionCode());
        out.appendPayload(mCondition, 2);
        return true;
    }
    return false;
}

void DreamcastController::reset()
{}

uint32_t DreamcastController::getFunctionDefinition()
{
    // This is mostly a guess
    return (
        (mEnabledControls.enLeftD  ? 0x000000F0 : 0) |
        (mEnabledControls.enRightD ? 0x0000F000 : 0) |
        (mEnabledControls.enLeftA  ? 0x000C0000 : 0) |
        (mEnabledControls.enRightA ? 0x00300000 : 0) |
        (mEnabledControls.enL      ? 0x00020000 : 0) |
        (mEnabledControls.enR      ? 0x00010000 : 0) |
        (mEnabledControls.enStart  ? 0x00000008 : 0) |
        (mEnabledControls.enA      ? 0x00000004 : 0) |
        (mEnabledControls.enB      ? 0x00000002 : 0) |
        (mEnabledControls.enC      ? 0x00000001 : 0) |
        (mEnabledControls.enD      ? 0x00000800 : 0) |
        (mEnabledControls.enX      ? 0x00000400 : 0) |
        (mEnabledControls.enY      ? 0x00000200 : 0) |
        (mEnabledControls.enZ      ? 0x00000100 : 0)
    );
}

void DreamcastController::setCondition(controller_condition_t condition)
{
    memcpy(mCondition, &condition, sizeof(mCondition));
}

void DreamcastController::setControls(const Controls& controls)
{
    controller_condition_t condition;
    condition.l = controls.l2;
    condition.r = controls.r2;
    condition.a = !controls.south;
    condition.b = !controls.east;
    condition.c = !controls.r1;
    condition.x = !controls.west;
    condition.y = !controls.north;
    condition.z = !controls.l1;
    condition.d = !controls.l3;
    condition.start = !controls.start;

    condition.up = 1;
    condition.down = 1;
    condition.left = 1;
    condition.right = 1;
    switch(controls.hat)
    {
        case Hat::UP_LEFT:
            condition.up = 0;
            condition.left = 0;
            break;

        case Hat::UP_RIGHT:
            condition.up = 0;
            condition.right = 0;
            break;

        case Hat::UP:
            condition.up = 0;
            break;

        case Hat::DOWN_LEFT:
            condition.down = 0;
            condition.left = 0;
            break;

        case Hat::DOWN_RIGHT:
            condition.down = 0;
            condition.right = 0;
            break;

        case Hat::DOWN:
            condition.down = 0;
            break;

        case Hat::LEFT:
            condition.left = 0;
            break;

        case Hat::RIGHT:
            condition.right = 0;
            break;

        case Hat::NEUTRAL:
        default:
            // Neutral
            break;
    }

    condition.rAnalogUD = controls.ry;
    condition.rAnalogLR = controls.rx;
    condition.lAnalogUD = controls.ly;
    condition.lAnalogLR = controls.lx;

    // No secondary D-pad
    condition.upb = 1;
    condition.downb = 1;
    condition.leftb = 1;
    condition.rightb = 1;

    setCondition(condition);
}

uint32_t DreamcastController::getConditionSamples()
{
    return mConditionSamples;
}

} // namespace client
