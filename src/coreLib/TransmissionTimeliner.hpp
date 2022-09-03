#pragma once

#include "MaplePacket.hpp"
#include "MapleBusInterface.hpp"
#include "PrioritizedTxScheduler.hpp"

class TransmissionTimeliner
{

public:
    //! Constructor
    //! @param[in] bus  The maple bus that scheduled transmissions are written to
    //! @param[in] schedule  The schedule to pop transmissions from
    TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule);

    //! Timeliner task - called periodically to process timeliner events
    //! @param[in] time  The current time task is run
    //! @returns the transmission that started or nullptr if nothing was transmitted
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> task(uint64_t time);

protected:
    //! The maple bus that scheduled transmissions are written to
    MapleBusInterface& mBus;
    //! The schedule that transmissions are popped from
    std::shared_ptr<PrioritizedTxScheduler> mSchedule;
    //! Recently popped transmission that is waiting to be sent
    std::shared_ptr<const PrioritizedTxScheduler::Transmission> mNextTx;
};