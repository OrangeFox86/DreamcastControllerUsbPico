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
    send(7, 10, 2000);
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

void DreamcastVibration::send(uint64_t timeUs, uint8_t power, int16_t inclination, uint32_t durationMs)
{
    // This is just a guesstimate of what is going on here based on trial and error...

    // Payload byte 2
    // Byte 0: duration (00 to FF)
    //         - This is some kind of duration that is augmented by bytes 1, 2, and 3
    // Byte 1: frequency (07 to 3B)
    //         - Lower values cause pulsation
    //         - The lower the value, the longer the duration
    //         - The higher the value, the greater the speed
    // Byte 2: intensity/ramp up/ramp down
    //         - 0X where X is:
    //            - 0-8 : Stable vibration (0: off, 1: low, 7: high)
    //            - 8-F : Ramp up, starting intensity up to max (8: off, 9: low, F: high)
    //         - X0 where X is:
    //            - 0-7 : Stable vibration (0: off, 1: low, 7: high)
    //            - 8-F : Ramp down, starting intensity down to min (8: off, 9: low, F: high)
    //         - X8 where X is:
    //            - 0-7 : Ramp up, starting intensity up to max (0: off, 1: low, 7: high)
    //         - 8X where X is:
    //            - 0-7 : Ramp down, starting intensity down to min (0: off, 1: low, 7: high)
    // Byte 3: 10 or 11 ???
    //         - Most sig nibble must be 1 for the command to be accepted (unless entire word is 0x00000000)
    //         - Least sig nibble must be 0 or 1 for the command to be accepted
    //         - The least significant nibble when set to 1: override to internal duration
    // A value of 0x00000000 will stop current vibration

    uint32_t vibrationWord = 0x10000000;

    // Limit power value to 7
    power = std::min(power, (uint8_t)MAX_POWER);
    // Set Power and inclination direction into word
    if (inclination > 0)
    {
        vibrationWord |= (power << 20) | 0x080000;
    }
    else if (inclination < 0)
    {
        vibrationWord |= (power << 16) | 0x800000;
        // Force inclination to be positive for the next step
        inclination = -inclination;
    }
    else
    {
        vibrationWord |= (power << 20);
    }

    // Determine frequency and inclination
    uint32_t freq = 0;
    if (inclination == 0)
    {
        // Can play with inclination value if it is 0 in order to set longer durations
        // I didn't spend much time deciding what seems best, but these are good enough
        if (durationMs > 30000)
        {
            freq = MIN_FREQ_VALUE;
        }
        else if (durationMs > 10000)
        {
            freq = 0x0F;
        }
        else if (durationMs > 5000)
        {
            freq = 0x28;
        }
        else
        {
            freq = MAX_FREQ_VALUE;
        }
        // I generated this equation based on observed vibration times
        inclination = std::min((uint32_t)((freq / 2 + 1) * (durationMs / 1000.0) - 1), MAX_INCLINATION);
    }
    else
    {
        // Limit inclination value to 255 (inclination should be positive because of above)
        inclination = std::min(inclination, (int16_t)MAX_INCLINATION);
        // Compute freq from all of the above and desired ms
        // I generated this equation based on observed vibration times
        freq = 2 * ((1000 * inclination * (MAX_POWER - power + 1) / durationMs - 1));
        freq = std::min(freq, MAX_FREQ_VALUE);
    }

    // Set frequency and inclination
    vibrationWord |= (freq << 8) | inclination;

    // Send it!
    uint32_t payload[2] = {FUNCTION_CODE, vibrationWord};
    mEndpointTxScheduler->add(
        timeUs,
        this,
        COMMAND_SET_CONDITION,
        payload,
        2,
        true,
        0);
}

void DreamcastVibration::send(uint8_t power, int16_t inclination, uint32_t durationMs)
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, power, inclination, durationMs);
}
