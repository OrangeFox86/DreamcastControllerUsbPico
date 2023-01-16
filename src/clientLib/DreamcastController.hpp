#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

namespace client
{
class DreamcastController : public DreamcastPeripheralFunction
{
public:
    inline DreamcastController() :
        DreamcastPeripheralFunction(DEVICE_FN_CONTROLLER)
    {}

    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        const uint8_t cmd = in.frame.command;
        if (cmd == COMMAND_GET_CONDITION)
        {
            out.frame.command = COMMAND_RESPONSE_DATA_XFER;
            out.reservePayload(3);
            out.appendPayload(getFunctionCode());
            // Controls in their neutral position
            uint32_t payload[2] = {0xFFFF0000, 0x80808080};
            out.appendPayload(payload, 2);
            return true;
        }
        return false;
    }

    inline virtual void reset() final
    {}

    inline virtual uint32_t getFunctionDefinition() final
    {
        return 0x000F06FE;
    }
};
}