#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"
#include "dreamcast_structures.h"
#include "GamepadHost.hpp"

namespace client
{
class DreamcastController : public DreamcastPeripheralFunction, public GamepadHost
{
public:
    DreamcastController();

    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    virtual void reset() final;

    virtual uint32_t getFunctionDefinition() final;

    void setCondition(controller_condition_t condition);

    virtual void setControls(const Controls& controls) final;

private:
    uint32_t mCondition[2];
};
}