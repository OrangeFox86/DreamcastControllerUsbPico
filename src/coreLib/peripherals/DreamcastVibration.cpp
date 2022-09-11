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
    send(7, 0, 0, 200);
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

uint8_t DreamcastVibration::computeNumIncrements(uint8_t power, int8_t inclination)
{
    // Compute the number of increments to execute
    if (inclination > 0)
    {
        return (MAX_POWER - power + 1);
    }
    else if (inclination < 0)
    {
        return  (power - MIN_POWER + 1);
    }
    else
    {
        return 1;
    }
}

uint8_t DreamcastVibration::freqSelect(uint8_t numIncrements, uint8_t pulsation, uint32_t durationMs)
{
    pulsation = std::min(pulsation, MAX_PULSATION_VALUE);

    // The adjusted duration value used for checking against limits
    uint32_t durationValue = (durationMs * numIncrements);

    // Compute the maximum frequency in order to achieve the given duration
    uint8_t maxFreq = 0;
    // I didn't spend much time deciding what seems best, but these are good enough
    if (durationValue < 8000)
    {
        maxFreq = MAX_FREQ_VALUE;
    }
    else if (durationValue < 12000)
    {
        maxFreq = 0x28;
    }
    else if (durationValue < 16000)
    {
        maxFreq = 0x1D;
    }
    else if (durationValue < 20000)
    {
        maxFreq = 0x17;
    }
    else if (durationValue < 24000)
    {
        maxFreq = 0x13;
    }
    else if (durationValue < 28000)
    {
        maxFreq = 0x10;
    }
    else if (durationValue < 33000)
    {
        maxFreq = 0x0D;
    }
    else if (durationValue < 35000)
    {
        maxFreq = 0x0C;
    }
    else if (durationValue < 38000)
    {
        maxFreq = 0x0B;
    }
    else if (durationValue < 41000)
    {
        maxFreq = 0x0A;
    }
    else if (durationValue < 45000)
    {
        maxFreq = 0x09;
    }
    else if (durationValue < 50000)
    {
        maxFreq = 0x08;
    }
    else
    {
        maxFreq = MIN_FREQ_VALUE;
    }

    // Compute the best frequency for the given pulsation value
    uint8_t freq = maxFreq;
    if (pulsation != 0)
    {
        // If pulsation is set, select the frequency that best gives us that amount of pulsation
        freq = (MAX_PULSATION_VALUE - pulsation) + MIN_FREQ_VALUE;
        if (durationValue < 100)
        {
            // Can't achieve this short duration with small freq
            // Not going to be on long enough to feel any pulsation anyway
            freq = std::max(freq, (uint8_t)0x13);
        }
        else if (durationValue < 250)
        {
            freq = std::max(freq, (uint8_t)0x08);
        }
        else
        {
            // Make sure the duration can be achieved
            freq = std::min(freq, maxFreq);
        }
    }
    return freq;
}

void DreamcastVibration::send(uint64_t timeUs, uint8_t power, int8_t inclination, uint8_t pulsation, uint32_t durationMs)
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
    // A value of 0x10000000 will stop current vibration

    // TODO: There is a block write command which sets the maximum vibration time. That is also what
    //       is used when byte 3 least sig nibble is set to 1. Implement that value somewhere.

    uint32_t vibrationWord = 0x10000000;

    if (power > 0 && durationMs > 0)
    {
        // Limit power value to 7
        power = std::min(power, (uint8_t)MAX_POWER);

        // Compute number of increments and frequency
        uint8_t numIncrements = computeNumIncrements(power, inclination);
        uint8_t freq = freqSelect(numIncrements, pulsation, durationMs);

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

        // I generated this equation based on observed vibration times
        // (it's close but not perfect - off by 5% to 10%)
        uint32_t durationValue = std::min(
            (uint32_t)(((freq / 2 + 1) * (durationMs / 1000.0)) / numIncrements),
            MAX_DURATION_VALUE);

        // Set frequency and duration
        vibrationWord |= (freq << 8) | durationValue;
    }

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

void DreamcastVibration::send(uint8_t power, int8_t inclination, uint8_t pulsation, uint32_t durationMs)
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, power, inclination, pulsation, durationMs);
}

void DreamcastVibration::stop()
{
    send(0, 0, 0, 0);
}
