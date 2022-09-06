#pragma once

#include "DreamcastPeripheral.hpp"
#include "MapleBusInterface.hpp"
#include "ScreenData.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast screen peripheral
class DreamcastScreen : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this screen
        DreamcastScreen(uint8_t addr,
                        std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                        PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastScreen();

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Called when transmission failed
        //! @param[in] writeFailed  Set to true iff TX failed because write failed
        //! @param[in] readFailed  Set to true iff TX failed because read failed
        //! @param[in] tx  The transmission that failed
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! @returns peripheral name
        virtual inline const char* getName() final{ return "screen"; }

    private:
        //! Time between each screen state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! Time which the next screen state poll will occur
        uint64_t mNextCheckTime;
        //! True iff the screen is waiting for data
        bool mWaitingForData;
        //! Initialized to true and set to true when write needs to be made even if screen hasn't
        //! been updated
        bool mUpdateRequired;
        //! Reference to screen data which is externally modified in internally read
        ScreenData& mScreenData;
        //! Transmission ID of the last screen
        uint32_t mTransmissionId;
};
