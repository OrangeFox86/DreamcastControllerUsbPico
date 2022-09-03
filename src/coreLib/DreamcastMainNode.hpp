#pragma once

#include "DreamcastNode.hpp"
#include "DreamcastSubNode.hpp"
#include "MapleBusInterface.hpp"
#include "DreamcastPeripheral.hpp"
#include "TransmissionTimeliner.hpp"

#include <memory>
#include <vector>

//! Handles communication for the main Dreamcast node for a single bus. In other words, this
//! facilitates communication to test for and identify a main peripheral such as a controller and
//! routes received communication to that peripheral or a sub node under this.
class DreamcastMainNode : public DreamcastNode
{
    public:
        //! Constructor
        //! @param[in] bus  The bus on which this node communicates
        //! @param[in] playerData  The player data passed to any connected peripheral
        //! @param[in] prioritizedTxScheduler  Scheduler to use for prioritizing transmissions
        DreamcastMainNode(MapleBusInterface& bus,
                          PlayerData playerData,
                          std::shared_ptr<PrioritizedTxScheduler> prioritizedTxScheduler);

        //! Virtual destructor
        virtual ~DreamcastMainNode();

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastNode
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

    private:
        //! Adds an auto reload info request to the transmission schedule
        void addInfoRequestToSchedule(uint64_t currentTimeUs = 0);

    public:
        //! Number of microseconds in between each info request when no peripheral is detected.
        //! Since main node is highest priority, this time needs to be long enough that lower
        //! priority messages fit in between my messages. 16 ms is what Dreamcast sets this to, and
        //! it's basically the fastest I can achieve while satisfying that requirement.
        static const uint32_t US_PER_CHECK = 16000;
        //! Time between each reset
        static const uint32_t US_PER_RESET = 1280000;
        //! Main node has highest priority
        static const uint8_t MAIN_TRANSMISSION_PRIORITY;
        //! Sub nodes have lower priority
        static const uint8_t SUB_TRANSMISSION_PRIORITY;

    protected:
        //! The bus on which this node communicates
        MapleBusInterface& mBus;
        //! The sub nodes under this node
        std::vector<std::shared_ptr<DreamcastSubNode>> mSubNodes;
        //! Executes transmissions from the schedule
        TransmissionTimeliner mTransmissionTimeliner;
        //! ID of the device info request auto reload transmission this object added to the schedule
        int64_t mScheduleId;
        //!
        int64_t mScheduleId2;
};