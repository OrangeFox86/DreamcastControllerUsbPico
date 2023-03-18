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

    //! Inherited from DreamcastPeripheralFunction
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Inherited from DreamcastPeripheralFunction
    virtual void reset() final;

    //! Inherited from DreamcastPeripheralFunction
    virtual uint32_t getFunctionDefinition() final;

    //! Sets the raw controller condition
    void setCondition(controller_condition_t condition);

    //! Sets the standard set of gamepad controls to the controller condition
    virtual void setControls(const Controls& controls) final;

    //! @returns the number of condition samples made
    uint32_t getConditionSamples();

private:
    //! Last set controller condition
    uint32_t mCondition[2];
    //! Number of condition samples requested by host
    uint32_t mConditionSamples;
};
}