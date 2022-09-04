#pragma once

#include "DreamcastPeripheral.hpp"
#include "MapleBusInterface.hpp"
#include "DreamcastControllerObserver.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast controller peripheral
class DreamcastController : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this controller
        DreamcastController(uint8_t addr, std::shared_ptr<EndpointTxSchedulerInterface> scheduler, PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastController();

        //! Inherited from DreamcastPeripheral
        virtual bool handleData(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual bool task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastPeripheral
        virtual void txSent(std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) final;

    private:
        //! Time between each controller state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! The gamepad to write button presses to
        DreamcastControllerObserver& mGamepad;
        //! Initialized to true and set to false when transmission fails
        bool mIsConnected;
        //! True iff the controller is waiting for data
        bool mWaitingForData;
        //! Initialized to true and set to false in task()
        bool mFirstTask;
        //! ID of the get condition transmission
        uint32_t mConditionTxId;
};
