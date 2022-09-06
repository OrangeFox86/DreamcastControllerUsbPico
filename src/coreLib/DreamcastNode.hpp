#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"
#include "DreamcastScreen.hpp"
#include "EndpointTxSchedulerInterface.hpp"
#include "Transmitter.hpp"

#include <stdint.h>
#include <vector>
#include <memory>

//! Base class for an addressable node on a Maple Bus
class DreamcastNode : public Transmitter
{
    public:
        //! Virtual destructor
        virtual ~DreamcastNode() {}

        //! Called periodically for this node to execute tasks for the given point in time
        //! @param[in] currentTimeUs  The current time in microseconds
        virtual void task(uint64_t currentTimeUs) = 0;

        //! @returns this node's address
        inline uint8_t getAddr() { return mAddr; }

        //! @returns recipient address for this node
        inline uint8_t getRecipientAddress()
        {
            return DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr);
        }

    protected:
        //! Main constructor with scheduler
        DreamcastNode(uint8_t addr,
                      std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                      PlayerData playerData) :
            mAddr(addr), mEndpointTxScheduler(scheduler), mPlayerData(playerData), mPeripherals()
        {}

        //! Copy constructor
        DreamcastNode(const DreamcastNode& rhs) :
            mAddr(rhs.mAddr),
            mEndpointTxScheduler(rhs.mEndpointTxScheduler),
            mPlayerData(rhs.mPlayerData),
            mPeripherals()
        {
            mPeripherals = rhs.mPeripherals;
        }

        //! Run all peripheral tasks
        //! @param[in] currentTimeUs  The current time in microseconds
        void handlePeripherals(uint64_t currentTimeUs)
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                (*iter)->task(currentTimeUs);
            }
        }

        //! Factory function which generates peripheral objects for the given function code mask
        //! @param[in] functionCode  The function code mask
        virtual uint32_t peripheralFactory(uint32_t functionCode)
        {
            uint32_t mask = 0;

            mPeripherals.clear();

            if (functionCode & DEVICE_FN_CONTROLLER)
            {
                mPeripherals.push_back(std::make_shared<DreamcastController>(mAddr, mEndpointTxScheduler, mPlayerData));
                mask |= DEVICE_FN_CONTROLLER;
            }

            if (functionCode & DEVICE_FN_LCD)
            {
                mPeripherals.push_back(std::make_shared<DreamcastScreen>(mAddr, mEndpointTxScheduler, mPlayerData));
                mask |= DEVICE_FN_LCD;
            }

            // TODO: handle other peripherals here

            return mask;
        }

        //! Prints all peripheral names
        inline void debugPrintPeripherals()
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                if (iter != mPeripherals.begin())
                {
                    DEBUG_PRINT(", ");
                }
                DEBUG_PRINT("%s", (*iter)->getName());
            }
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
        const std::shared_ptr<EndpointTxSchedulerInterface> mEndpointTxScheduler;
        //! Player data on this node
        PlayerData mPlayerData;
        //! The connected peripherals addressed to this node (usually 0 to 2 items)
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripherals;
};