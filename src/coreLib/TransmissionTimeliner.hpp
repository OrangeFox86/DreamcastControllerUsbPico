#pragma once

#include "MaplePacket.hpp"
#include "MapleBusInterface.hpp"
#include "PrioritizedTxScheduler.hpp"

class TransmissionTimeliner
{
public:
    //! Status of the task
    struct ReadStatus
    {
        //! Set to received packet or nullptr if nothing received
        std::shared_ptr<const MaplePacket> received;
        //! Set to true iff transmission failed
        bool lastTxFailed;
        //! Set to true iff read failed
        bool lastRxFailed;

        ReadStatus() : received(nullptr), lastTxFailed(false), lastRxFailed(false)
        {}
    };

public:
    //! Constructor
    //! @param[in] bus  The maple bus that scheduled transmissions are written to
    //! @param[in] schedule  The schedule to pop transmissions from
    TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule);

    //! Read timeliner task - called periodically to process timeliner read events
    //! @param[in] currentTimeUs  The current time task is run
    //! @returns read status information
    ReadStatus readTask(uint64_t currentTimeUs);

    //! Write timeliner task - called periodically to process timeliner write events
    //! @param[in] currentTimeUs  The current time task is run
    //! @returns the transmission that started or nullptr if nothing was transmitted
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> writeTask(uint64_t currentTimeUs);

protected:
    //! The maple bus that scheduled transmissions are written to
    MapleBusInterface& mBus;
    //! The schedule that transmissions are popped from
    std::shared_ptr<PrioritizedTxScheduler> mSchedule;
    //! Recently popped transmission that is waiting to be sent
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> mNextTx;
};