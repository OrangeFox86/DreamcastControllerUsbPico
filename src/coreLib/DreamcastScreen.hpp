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
        virtual bool handleData(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual bool task(uint64_t currentTimeUs) final;

    private:
        //! Number of times failed communication occurs before determining that the screen is
        //! disconnected
        static const uint32_t NO_DATA_DISCONNECT_COUNT = 5;
        //! Time between each screen state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! Time which the next screen state poll will occur
        uint64_t mNextCheckTime;
        //! True iff the screen is waiting for data
        bool mWaitingForData;
        //! Number of consecutive times no data was received
        uint32_t mNoDataCount;
        //! Initialized to true and set to true when write needs to be made even if screen hasn't
        //! been updated
        bool mUpdateRequired;
        //! Reference to screen data which is externally modified in internally read
        ScreenData& mScreenData;
        //! Transmission ID of the last screen
        uint32_t mTransmissionId;
};
