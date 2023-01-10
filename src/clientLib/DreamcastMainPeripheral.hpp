#pragma once

#include <stdint.h>
#include <memory>
#include <map>

#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheral.hpp"

namespace client
{

class DreamcastMainPeripheral : public DreamcastPeripheral
{
public:
    DreamcastMainPeripheral(uint8_t addr,
                            uint8_t regionCode,
                            uint8_t connectionDirectionCode,
                            const char* descriptionStr,
                            const char* producerStr,
                            const char* versionStr,
                            float standbyCurrentmA,
                            float maxCurrentmA);

    DreamcastMainPeripheral() = delete;

    //! Add a sub-peripheral to this peripheral (only valid of main peripheral)
    void addSubPeripheral(std::shared_ptr<DreamcastPeripheral> subPeripheral);

    //! Handle packet that is meant for me
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Deliver given packet to target peripheral
    //! @param[in] payload  The received payload buffer
    //! @param[in] payloadLen  The length of payload
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @return true iff the packet was dispensed and handled
    bool dispensePacket(const uint32_t* payload, uint32_t payloadLen, MaplePacket& out);

    virtual void reset() final;

private:
    void setPlayerIndex(uint8_t idx);

public:

private:
    uint8_t mPlayerIndex;
    std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>> mSubPeripherals;
};

}
