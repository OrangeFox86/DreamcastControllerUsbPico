#pragma once

#include <stdint.h>
#include <memory>
#include <map>

#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheral.hpp"

namespace client
{

class DreamcastMainPeripheral : public DreamcastPeripheral
{
public:
    DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                            uint8_t addr,
                            uint8_t regionCode,
                            uint8_t connectionDirectionCode,
                            const char* descriptionStr,
                            const char* producerStr,
                            const char* versionStr,
                            float standbyCurrentmA,
                            float maxCurrentmA);

    DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                            uint8_t addr,
                            uint8_t regionCode,
                            uint8_t connectionDirectionCode,
                            const char* descriptionStr,
                            const char* versionStr,
                            float standbyCurrentmA,
                            float maxCurrentmA);

    DreamcastMainPeripheral() = delete;

    //! Add a sub-peripheral to this peripheral (only valid of main peripheral)
    void addSubPeripheral(std::shared_ptr<DreamcastPeripheral> subPeripheral);

    //! Handle packet that is meant for me
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Deliver given packet to target peripheral
    //! @param[in] in  The packet read from the Maple Bus
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @return true iff the packet was dispensed and handled
    bool dispensePacket(const MaplePacket& in, MaplePacket& out);

    virtual void reset() final;

    //! Must be called periodically to process communication
    //! @param[in] currentTimeUs  The current system time in microseconds
    void task(uint64_t currentTimeUs);

private:
    void setPlayerIndex(uint8_t idx);

public:
    //! Maple Bus read timeout in microseconds
    static const uint64_t READ_TIMEOUT_US = 1000000;

private:
    const std::shared_ptr<MapleBusInterface> mBus;
    uint8_t mPlayerIndex;
    std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>> mSubPeripherals;

    uint8_t mLastSender;
    MaplePacket mPacketOut;
    MaplePacket mLastPacketOut;
    bool mPacketSent;
    MaplePacket mPacketIn;
};

}
