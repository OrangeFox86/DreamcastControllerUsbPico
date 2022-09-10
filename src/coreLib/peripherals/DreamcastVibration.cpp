#include "DreamcastVibration.hpp"
#include "utils.h"
#include <algorithm>

DreamcastVibration::DreamcastVibration(uint8_t addr,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("vibration", addr, scheduler, playerData.playerIndex),
    mTransmissionId(0)
{
    // Send some vibrations on connection
    send(7, 10, 5);
}

DreamcastVibration::~DreamcastVibration()
{}

void DreamcastVibration::task(uint64_t currentTimeUs)
{
}

void DreamcastVibration::txStarted(std::shared_ptr<const Transmission> tx)
{
}

void DreamcastVibration::txFailed(bool writeFailed,
                                  bool readFailed,
                                  std::shared_ptr<const Transmission> tx)
{
}

void DreamcastVibration::txComplete(std::shared_ptr<const MaplePacket> packet,
                                    std::shared_ptr<const Transmission> tx)
{
}

void DreamcastVibration::send(uint64_t timeUs, uint8_t intensity, uint8_t speed, uint8_t revolutions)
{
    // This is just a guesstimate of what is going on here based on trial and error...

    // Payload byte 2
    // Byte 0: number of revolutions (00 to FF)
    // Byte 1: undulation/speed (07 to 3B)
    // Byte 2: intensity/speed (01-07, 09-0F, 1X-7X)
    // Byte 3: 10 or 11 ???

    // Last byte with byte 2 at 04
    // 0xFF: 0 seconds

    // Last byte with byte 2 at 06
    // 0xFF: 0 seconds

    // Last byte with byte 2 at 07
    // 0xFF: 60 seconds (w undulation)

    // Last byte with byte 2 at 08
    // 0xFF: 53.5 seconds (w undulation)

    // Last byte: (~0.117 seconds per div) with byte 2 at 0F
    // 0x08: 1 second
    // 0x10: 1.73 seconds
    // 0x20: 3.91 seconds
    // 0x40: 7.42 seconds
    // 0x80: 15 seconds
    // 0xFF: 30 seconds

    // Last byte with byte 2 at 20
    // 0xFF: 14.76 seconds

    // Last byte with byte 2 at 30
    // 0xFF: 9.69 seconds

    // Last byte with byte 2 at 38
    // 0xFF: 8.5 seconds

    // Last byte with byte 2 at 3A
    // 0xFF: 8 seconds

    // Last byte with byte 2 at 3B
    // 0xFF: 8.2 seconds

    // Last byte with byte 2 at 3C
    // 0xFF: 0 seconds

    // Last byte with byte 2 at 3F
    // 0xFF: 0 seconds

    intensity = std::min(intensity, (uint8_t)7);
    speed = std::min(speed, (uint8_t)59);
    uint32_t payload[2] = {
        FUNCTION_CODE,
        0x10000000 | ((uint32_t)intensity << 20) | ((uint32_t)speed << 8) | (uint32_t)revolutions
    };
    mEndpointTxScheduler->add(
        timeUs,
        this,
        COMMAND_SET_CONDITION,
        payload,
        2,
        true,
        0);
}

void DreamcastVibration::send(uint8_t intensity, uint8_t speed, uint8_t revolutions)
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, intensity, speed, revolutions);
}
