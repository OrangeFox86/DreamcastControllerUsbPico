#include "DreamcastVibration.hpp"
#include "utils.h"
#include <algorithm>

// I generated these equations based on observed vibrations with a test device

// Converts pulsation frequency value into actual pulsation frequency in Hz
#define PULSATION_FREQ(freqValue) (freqValue / 2.0 + 1)

// Duration value for given increments, frequency, and duration
#define COMPUTE_CYCLES(numIncrements, freqValue, durationMs) (                                      \
    (uint8_t)limit_value(                                                                           \
        (int32_t)((PULSATION_FREQ(freqValue) * (durationMs / 1000.0)) / numIncrements - 1 + 0.5),   \
        (int32_t)MIN_CYCLES,                                                                        \
        (int32_t)MAX_CYCLES)                                                                        \
)

#define FREQ_IDX_TO_FREQ(freqIdx) (freqIdx + MIN_FREQ_VALUE)
// Maximum duration for a single increment
#define MAX_DURATION_IDX(freqIdx) (uint32_t)(1000 * (MAX_CYCLES + 1) / PULSATION_FREQ(FREQ_IDX_TO_FREQ(freqIdx)))

// There are ways of generating this like using BOOST_PP_ENUM, but I don't want to include boost just for that
const uint32_t DreamcastVibration::MAX_DURATION_LOOKUP[NUM_FREQ_VALUES] =
{
    MAX_DURATION_IDX(0),  MAX_DURATION_IDX(1),  MAX_DURATION_IDX(2),  MAX_DURATION_IDX(3),
    MAX_DURATION_IDX(4),  MAX_DURATION_IDX(5),  MAX_DURATION_IDX(6),  MAX_DURATION_IDX(7),
    MAX_DURATION_IDX(8),  MAX_DURATION_IDX(9),  MAX_DURATION_IDX(10), MAX_DURATION_IDX(11),
    MAX_DURATION_IDX(12), MAX_DURATION_IDX(13), MAX_DURATION_IDX(14), MAX_DURATION_IDX(15),
    MAX_DURATION_IDX(16), MAX_DURATION_IDX(17), MAX_DURATION_IDX(18), MAX_DURATION_IDX(19),
    MAX_DURATION_IDX(20), MAX_DURATION_IDX(21), MAX_DURATION_IDX(22), MAX_DURATION_IDX(23),
    MAX_DURATION_IDX(24), MAX_DURATION_IDX(25), MAX_DURATION_IDX(26), MAX_DURATION_IDX(27),
    MAX_DURATION_IDX(28), MAX_DURATION_IDX(29), MAX_DURATION_IDX(30), MAX_DURATION_IDX(31),
    MAX_DURATION_IDX(32), MAX_DURATION_IDX(33), MAX_DURATION_IDX(34), MAX_DURATION_IDX(35),
    MAX_DURATION_IDX(36), MAX_DURATION_IDX(37), MAX_DURATION_IDX(38), MAX_DURATION_IDX(39),
    MAX_DURATION_IDX(40), MAX_DURATION_IDX(41), MAX_DURATION_IDX(42), MAX_DURATION_IDX(43),
    MAX_DURATION_IDX(44), MAX_DURATION_IDX(45), MAX_DURATION_IDX(46), MAX_DURATION_IDX(47),
    MAX_DURATION_IDX(48), MAX_DURATION_IDX(49), MAX_DURATION_IDX(50), MAX_DURATION_IDX(51),
    MAX_DURATION_IDX(52)
};

DreamcastVibration::DreamcastVibration(uint8_t addr,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("vibration", addr, scheduler, playerData.playerIndex),
    mTransmissionId(0)
{
    // Send some vibrations on connection
    send(6, 0, 15, 100);
}

DreamcastVibration::~DreamcastVibration()
{}

void DreamcastVibration::task(uint64_t currentTimeUs)
{
}

void DreamcastVibration::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (tx->transmissionId == mTransmissionId)
    {
        mTransmissionId = 0;
    }
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
        // Increase from given power to max power
        return (MAX_POWER - power + 1);
    }
    else if (inclination < 0)
    {
        // Decrease from given power to min power
        return  (power - MIN_POWER + 1);
    }
    else
    {
        return 1;
    }
}

uint8_t DreamcastVibration::maxFreqForDuration(uint8_t numIncrements, uint32_t durationMs)
{
    // Compute the maximum frequency in order to achieve the given duration
    for (uint32_t i = NUM_FREQ_VALUES; i > 0; --i)
    {
        if (durationMs <= (MAX_DURATION_LOOKUP[i - 1] * numIncrements))
        {
            return FREQ_IDX_TO_FREQ(i - 1);
        }
    }

    // Can't achieve this duration, so just return min to get the most out of it
    return MIN_FREQ_VALUE;
}

void DreamcastVibration::send(uint64_t timeUs, uint8_t power, int8_t inclination, uint8_t desiredFreq, uint32_t durationMs)
{
    // This is just a guesstimate of what is going on here based on trial and error...

    // Payload byte 2
    // Byte 0: cycles (00 to FF)
    //         - The number of pulsation cycles per ramp increment
    // Byte 1: pulsation frequency value (07 to 3B)
    //         - Lower values cause noticeable pulsation
    //         - The lower the value, the longer the total vibration duration
    //         - The frequency seems to correlate to about (value / 2 + 1) Hz
    // Byte 2: intensity/ramp up/ramp down
    //         - Each intensity is executed for the number of specified cycles
    //         - 0X where X is:
    //            - 0-8 : Single stable vibration (0: off, 1: low, 7: high)
    //            - 8-F : Ramp up, starting intensity up to max (8: off, 9: low, F: high)
    //         - X0 where X is:
    //            - 0-7 : Single stable vibration (0: off, 1: low, 7: high)
    //            - 8-F : Ramp down, starting intensity down to min (8: off, 9: low, F: high)
    //         - X8 where X is:
    //            - 0-7 : Ramp up, starting intensity up to max (0: off, 1: low, 7: high)
    //         - 8X where X is:
    //            - 0-7 : Ramp down, starting intensity down to min (0: off, 1: low, 7: high)
    // Byte 3: 10 or 11 ???
    //         - Most sig nibble must be 1 for the command to be accepted
    //         - Least sig nibble must be 0 or 1 for the command to be accepted
    //         - The least significant nibble when set to 1: augments total duration
    // A value of 0x10000000 will stop current vibration

    uint32_t vibrationWord = 0x10000000;

    if (power > 0 && durationMs > 0)
    {
        // Limit power value to valid value
        power = limit_value(power, MIN_POWER, MAX_POWER);

        // Compute number of increments and frequency
        uint8_t numIncrements = computeNumIncrements(power, inclination);
        uint8_t freq = MAX_FREQ_VALUE;
        if (desiredFreq == 0)
        {
            // Get the maximum frequency that can achieve the given duration
            freq = maxFreqForDuration(numIncrements, durationMs);
        }
        else
        {
            // Just make sure the value fits within limits
            freq = limit_value(desiredFreq, MIN_FREQ_VALUE, MAX_FREQ_VALUE);
        }

        // Set Power and inclination direction into word
        if (inclination > 0)
        {
            vibrationWord |= (power << 20) | 0x080000;
        }
        else if (inclination < 0)
        {
            vibrationWord |= (power << 16) | 0x800000;
        }
        else
        {
            vibrationWord |= (power << 20);
        }

        // Compute duration and limit to max value
        uint8_t durationValue = COMPUTE_CYCLES(numIncrements, freq, durationMs);

        // Set frequency and duration
        vibrationWord |= (freq << 8) | durationValue;
    }

    // Remove past transmission if it hasn't been sent yet
    if (mTransmissionId > 0)
    {
        mEndpointTxScheduler->cancelById(mTransmissionId);
        mTransmissionId = 0;
    }

    // Send it!
    uint32_t payload[2] = {FUNCTION_CODE, vibrationWord};
    mTransmissionId = mEndpointTxScheduler->add(
        timeUs,
        this,
        COMMAND_SET_CONDITION,
        payload,
        2,
        true,
        0);
}

void DreamcastVibration::send(uint8_t power, int8_t inclination, uint8_t desiredFreq, uint32_t durationMs)
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, power, inclination, desiredFreq, durationMs);
}

void DreamcastVibration::stop()
{
    send(0, 0, 0, 0);
}
