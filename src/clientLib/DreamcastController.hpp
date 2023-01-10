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
        const uint8_t cmd = in.getFrameCommand();
        if (cmd == COMMAND_GET_CONDITION)
        {
            out.setCommand(COMMAND_RESPONSE_DATA_XFER);
            out.setRecipientAddress(in.getFrameSenderAddr());
            uint32_t payload[3] = {getFunctionCode(), 0xFFFF0000, 0x80808080};
            out.setPayload(payload, 3);
            out.updateFrameLength();
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