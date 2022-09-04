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
        DreamcastMainNode(MapleBusInterface& bus,
                          PlayerData playerData,
                          std::shared_ptr<PrioritizedTxScheduler> prioritizedTxScheduler);

        //! Virtual destructor
        virtual ~DreamcastMainNode();

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastNode
        virtual bool handleData(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) final;

    private:
        //! Adds an auto reload info request to the transmission schedule
        void addInfoRequestToSchedule(uint64_t currentTimeUs = 0);

    public:
        //! Number of microseconds in between each info request when no peripheral is detected
        static const uint32_t US_PER_CHECK = 16000;
        //! Main node has highest priority
        static const uint8_t MAIN_TRANSMISSION_PRIORITY;
        //! Sub nodes have lower priority
        static const uint8_t SUB_TRANSMISSION_PRIORITY;

    protected:
        //! The sub nodes under this node
        std::vector<std::shared_ptr<DreamcastSubNode>> mSubNodes;
        //! Executes transmissions from the schedule
        TransmissionTimeliner mTransmissionTimeliner;
        //! ID of the device info request auto reload transmission this object added to the schedule
        int64_t mScheduleId;
};