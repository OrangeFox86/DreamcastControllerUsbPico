#pragma once

#include "DreamcastNode.hpp"
#include "DreamcastSubNode.hpp"
#include "MapleBusInterface.hpp"
#include "DreamcastPeripheral.hpp"

#include <memory>
#include <vector>

class DreamcastMainNode : public DreamcastNode
{
    public:
        DreamcastMainNode(MapleBusInterface& bus,
                          PlayerData playerData,
                          uint32_t numSubNodes = DreamcastPeripheral::MAX_SUB_PERIPHERALS);
        virtual ~DreamcastMainNode();

        virtual void task(uint64_t currentTimeUs) final;

        //! Handles incoming data destined for this node
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

    public:
        static const uint32_t US_PER_CHECK = 16000;
    protected:
        uint64_t mNextCheckTime;
        std::vector<std::shared_ptr<DreamcastSubNode>> mSubNodes;
};