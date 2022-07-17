#pragma once

#include "DreamcastNode.hpp"

class DreamcastSubNode : public DreamcastNode
{
    public:
        DreamcastSubNode(uint8_t addr, MapleBusInterface& bus, PlayerData playerData);
        DreamcastSubNode(const DreamcastSubNode& rhs);

        //! Handles incoming data destined for this device
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload);

        virtual void task(uint64_t currentTimeUs);

        virtual void mainPeripheralDisconnected();

        virtual void setConnected(bool connected);

    private:
        static const uint32_t US_PER_CHECK = 16000;
        static const uint32_t NUM_FAIL_COM_DISCONNECT = 5;
        uint64_t mNextCheckTime;
        bool mConnected;

};
