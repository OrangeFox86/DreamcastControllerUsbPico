#include "DreamcastVibration.hpp"
#include "utils.h"
#include <algorithm>

// Duration value for given increments, frequency, and duration
// I generated this equation based on observed vibration times with a test device
#define COMPUTE_DURATION_VALUE(numIncrements, freq, durationMs) (                       \
    (uint8_t)std::max(                                                                  \
        std::min(                                                                       \
            (int32_t)(((freq / 2.0 + 1) * (durationMs / 1000.0)) / numIncrements) - 1,  \
            (int32_t)MAX_DURATION_VALUE),                                               \
        (int32_t)0)                                                                     \
)
#define FREQ_IDX_TO_FREQ(freqIdx) (freqIdx + MIN_FREQ_VALUE)
#define FREQ_TO_FREQ_IDX(freq) (freq - MIN_FREQ_VALUE)
// Maximum duration for a single increment
#define COMPUTE_MAX_DURATION_MS(freqIdx) (uint32_t)(1000 * (MAX_DURATION_VALUE + 1) / (FREQ_IDX_TO_FREQ(freqIdx) / 2.0 + 1))

// There are ways of generating this like using BOOST_PP_ENUM, but I don't want to include boost just for that
const uint32_t DreamcastVibration::MAX_DURATION_LOOKUP[NUM_FREQ_VALUES] =
{
    COMPUTE_MAX_DURATION_MS(0),  COMPUTE_MAX_DURATION_MS(1),  COMPUTE_MAX_DURATION_MS(2),  COMPUTE_MAX_DURATION_MS(3),
    COMPUTE_MAX_DURATION_MS(4),  COMPUTE_MAX_DURATION_MS(5),  COMPUTE_MAX_DURATION_MS(6),  COMPUTE_MAX_DURATION_MS(7),
    COMPUTE_MAX_DURATION_MS(8),  COMPUTE_MAX_DURATION_MS(9),  COMPUTE_MAX_DURATION_MS(10), COMPUTE_MAX_DURATION_MS(11),
    COMPUTE_MAX_DURATION_MS(12), COMPUTE_MAX_DURATION_MS(13), COMPUTE_MAX_DURATION_MS(14), COMPUTE_MAX_DURATION_MS(15),
    COMPUTE_MAX_DURATION_MS(16), COMPUTE_MAX_DURATION_MS(17), COMPUTE_MAX_DURATION_MS(18), COMPUTE_MAX_DURATION_MS(19),
    COMPUTE_MAX_DURATION_MS(20), COMPUTE_MAX_DURATION_MS(21), COMPUTE_MAX_DURATION_MS(22), COMPUTE_MAX_DURATION_MS(23),
    COMPUTE_MAX_DURATION_MS(24), COMPUTE_MAX_DURATION_MS(25), COMPUTE_MAX_DURATION_MS(26), COMPUTE_MAX_DURATION_MS(27),
    COMPUTE_MAX_DURATION_MS(28), COMPUTE_MAX_DURATION_MS(29), COMPUTE_MAX_DURATION_MS(30), COMPUTE_MAX_DURATION_MS(31),
    COMPUTE_MAX_DURATION_MS(32), COMPUTE_MAX_DURATION_MS(33), COMPUTE_MAX_DURATION_MS(34), COMPUTE_MAX_DURATION_MS(35),
    COMPUTE_MAX_DURATION_MS(36), COMPUTE_MAX_DURATION_MS(37), COMPUTE_MAX_DURATION_MS(38), COMPUTE_MAX_DURATION_MS(39),
    COMPUTE_MAX_DURATION_MS(40), COMPUTE_MAX_DURATION_MS(41), COMPUTE_MAX_DURATION_MS(42), COMPUTE_MAX_DURATION_MS(43),
    COMPUTE_MAX_DURATION_MS(44), COMPUTE_MAX_DURATION_MS(45), COMPUTE_MAX_DURATION_MS(46), COMPUTE_MAX_DURATION_MS(47),
    COMPUTE_MAX_DURATION_MS(48), COMPUTE_MAX_DURATION_MS(49), COMPUTE_MAX_DURATION_MS(50), COMPUTE_MAX_DURATION_MS(51),
    COMPUTE_MAX_DURATION_MS(52)
};

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

uint8_t DreamcastVibration::freqSelect(uint8_t numIncrements, uint8_t pulsation, uint32_t durationMs)
{
    pulsation = std::min(pulsation, MAX_PULSATION_VALUE);

    // The adjusted duration value used for checking against limits
    uint32_t durationValue = (durationMs * numIncrements);

    // Compute the maximum frequency in order to achieve the given duration
    uint8_t maxFreq = MAX_FREQ_VALUE;
    for (uint32_t i = NUM_FREQ_VALUES; i > 0; --i)
    {
        if (durationValue <= MAX_DURATION_LOOKUP[i - 1])
        {
            maxFreq = FREQ_IDX_TO_FREQ(i - 1);
            break;
        }
    }

    // Compute the best frequency for the given pulsation value
    uint8_t freq = maxFreq;
    if (pulsation != 0)
    {
        // If pulsation is set, select the frequency that best gives us that amount of pulsation
        freq = (MAX_PULSATION_VALUE - pulsation) + MIN_FREQ_VALUE;
        if (durationValue < 250)
        {
            // Can't achieve this short duration with small freq
            // Not going to be on long enough to feel any pulsation anyway
            freq = std::max(freq, (uint8_t)0x13);
        }
        else
        {
            // Make sure the duration can be achieved
            freq = std::min(freq, maxFreq);
        }
    }
    // else: use max freq (initialized value) to limit pulsation

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
    //         - The least significant nibble when set to 1: augments duration
    // A value of 0x10000000 will stop current vibration

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
        }
        else
        {
            vibrationWord |= (power << 20);
        }

        // Compute duration and limit to max value
        uint8_t durationValue = COMPUTE_DURATION_VALUE(numIncrements, freq, durationMs);

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

void DreamcastVibration::send(uint8_t power, int8_t inclination, uint8_t pulsation, uint32_t durationMs)
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, power, inclination, pulsation, durationMs);
}

void DreamcastVibration::stop()
{
    send(0, 0, 0, 0);
}
