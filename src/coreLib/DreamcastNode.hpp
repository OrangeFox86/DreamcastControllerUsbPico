#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"

#include <stdint.h>
#include <vector>
#include <memory>

//! Base class for an addressable node on a Maple Bus
class DreamcastNode
{
    public:
        //! Virtual destructor
        virtual ~DreamcastNode() {}

        //! Handles incoming data destined for this node
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) = 0;

        virtual void task(uint64_t currentTimeUs) = 0;

        //! @returns this node's address
        inline uint8_t getAddr() { return mAddr; }

    protected:
        //! Main constructor
        DreamcastNode(uint8_t addr, MapleBusInterface& bus, PlayerData playerData) :
            mAddr(addr), mBus(bus), mPlayerData(playerData), mPeripherals()
        {}

        //! Copy constructor
        DreamcastNode(const DreamcastNode& rhs) :
            mAddr(rhs.mAddr),
            mBus(rhs.mBus),
            mPlayerData(rhs.mPlayerData),
            mPeripherals()
        {
            mPeripherals = rhs.mPeripherals;
        }

        //! Run all peripheral tasks
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @return true if all peripherals connected; false if all have disconnected
        bool handlePeripherals(uint64_t currentTimeUs)
        {
            bool connected = true;
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end() && connected;
                 ++iter)
            {
                if (!(*iter)->task(currentTimeUs))
                {
                    connected = false;
                }
            }

            if (!connected)
            {
                // One peripheral is no longer responding, so remove all
                mPeripherals.clear();
            }

            return connected;
        }

        //! Try to get peripherals to handle the given data
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        bool handlePeripheralData(uint8_t len,
                                  uint8_t cmd,
                                  const uint32_t *payload)
        {
            bool handled = false;
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end() && !handled;
                 ++iter)
            {
                handled = (*iter)->handleData(len, cmd, payload);
            }
            return handled;
        }

        virtual void peripheralFactory(uint32_t functionCode)
        {
            mPeripherals.clear();

            if (functionCode & DEVICE_FN_CONTROLLER)
            {
                mPeripherals.push_back(std::make_shared<DreamcastController>(mAddr, mBus, mPlayerData));
            }
            // TODO: handle other peripherals here
        }

    private:
        //! Default constructor - not implemented
        DreamcastNode();

    protected:
        //! Maximum number of players
        static const uint32_t MAX_NUM_PLAYERS = 4;
        //! Address of this node
        const uint8_t mAddr;
        //! The bus that this node communicates on
        MapleBusInterface& mBus;
        //! Player data on this node
        PlayerData mPlayerData;
        //! The connected peripherals addressed to this node (usually 0 to 2 items)
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripherals;
};