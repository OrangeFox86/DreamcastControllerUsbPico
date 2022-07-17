#pragma once

#include "DreamcastNode.hpp"
#include "DreamcastSubNode.hpp"
#include "MapleBusInterface.hpp"
#include "DreamcastPeripheral.hpp"

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
        DreamcastMainNode(MapleBusInterface& bus, PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastMainNode();

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastNode
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

    public:
        //! Number of microseconds in between each info request when no peripheral is detected
        static const uint32_t US_PER_CHECK = 16000;

    protected:
        //! The clock time of the next info request when no peripheral is detected
        uint64_t mNextCheckTime;
        //! The sub nodes under this node
        std::vector<std::shared_ptr<DreamcastSubNode>> mSubNodes;
};