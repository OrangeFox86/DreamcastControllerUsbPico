#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"
#include "DreamcastScreen.hpp"
#include "PrioritizedTxScheduler.hpp"

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

        //! Called periodically for this node to execute tasks for the given point in time
        //! @param[in] currentTimeUs  The current time in microseconds
        virtual void task(uint64_t currentTimeUs) = 0;

        //! @returns this node's address
        inline uint8_t getAddr() { return mAddr; }

    protected:
        //! Main constructor with scheduler
        DreamcastNode(uint8_t addr,
                      std::shared_ptr<PrioritizedTxScheduler> scheduler,
                      PlayerData playerData) :
            mAddr(addr), mPrioritizedTxScheduler(scheduler), mPlayerData(playerData), mPeripherals()
        {}

        //! Main constructor without scheduler
        DreamcastNode(uint8_t addr, PlayerData playerData) :
            mAddr(addr),
            mPrioritizedTxScheduler(std::make_shared<PrioritizedTxScheduler>()),
            mPlayerData(playerData),
            mPeripherals()
        {}

        //! Copy constructor
        DreamcastNode(const DreamcastNode& rhs) :
            mAddr(rhs.mAddr),
            mPrioritizedTxScheduler(rhs.mPrioritizedTxScheduler),
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

        //! Factory function which generates peripheral objects for the given function code mask
        //! @param[in] functionCode  The function code mask
        virtual void peripheralFactory(uint32_t functionCode)
        {
            mPeripherals.clear();

            if (functionCode & DEVICE_FN_CONTROLLER)
            {
                mPeripherals.push_back(std::make_shared<DreamcastController>(mAddr, *mPrioritizedTxScheduler, mPlayerData));
            }
            else if (functionCode & DEVICE_FN_LCD)
            {
                mPeripherals.push_back(std::make_shared<DreamcastScreen>(mAddr, *mPrioritizedTxScheduler, mPlayerData));
            }
            // TODO: handle other peripherals here
            // TODO: add a stub peripheral if none were created
        }

        //! @returns recipient address for this node
        inline uint8_t getRecipientAddress()
        {
            return DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr);
        }

    private:
        //! Default constructor - not implemented
        DreamcastNode() = delete;

    protected:
        //! Maximum number of players
        static const uint32_t MAX_NUM_PLAYERS = 4;
        //! Address of this node
        const uint8_t mAddr;
        //! Keeps all scheduled transmissions for my bus
        const std::shared_ptr<PrioritizedTxScheduler> mPrioritizedTxScheduler;
        //! Player data on this node
        PlayerData mPlayerData;
        //! The connected peripherals addressed to this node (usually 0 to 2 items)
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripherals;
};