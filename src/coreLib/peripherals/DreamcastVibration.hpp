#pragma once

#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast vibration peripheral
class DreamcastVibration : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this vibration device
        DreamcastVibration(uint8_t addr,
                        std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                        PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastVibration();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        //! Sends vibration
        //! @param[in] timeUs  The time to send vibration (optional)
        //! @param[in] power  Starting power intensity [0,7] (0 to disable completely)
        //! @param[in] inclination  -1: ramp down, 0: constant, 1: ramp up
        //! @param[in] pulsation  The amount of pulsation to apply [0, 32] (0 for none, 32 for max)
        //! @param[in] durationMs  The length of time in ms to vibrate (0 to disable completely)
        void send(uint64_t timeUs, uint8_t power, int8_t inclination, uint8_t pulsation, uint32_t durationMs);
        void send(uint8_t power, int8_t inclination, uint8_t pulsation, uint32_t durationMs);

        //! Stops current vibration
        void stop();

    private:
        //! Computes the number of power increments that will be executed
        //! @param[in] power  Starting power intensity [1,7]
        //! @param[in] inclination  -1: ramp down, 0: constant, 1: ramp up
        //! @returns the number of power increments
        uint8_t computeNumIncrements(uint8_t power, int8_t inclination);

        //! Selects the frequency best suited to achieve the given parameters
        //! @param[in] numIncrements  The number of power increments that will be executed
        //! @param[in] pulsation  The amount of pulsation to apply [0, 32] (0 for none, 32 for max)
        //! @returns the selected frequency value
        uint8_t freqSelect(uint8_t numIncrements, uint8_t pulsation, uint32_t durationMs);

    public:
        //! Function code for screen
        static const uint32_t FUNCTION_CODE = DEVICE_FN_VIBRATION;
        static const uint32_t MAX_FREQ_VALUE = 0x3B;
        static const uint32_t MIN_FREQ_VALUE = 0x07;
        static const uint32_t MAX_DURATION_VALUE = 0xFF;
        static const uint32_t MAX_POWER = 0x07;
        static const uint32_t MIN_POWER = 0x01;
        static const uint8_t MAX_PULSATION_VALUE = 32;

    private:
        uint32_t mTransmissionId;
};
