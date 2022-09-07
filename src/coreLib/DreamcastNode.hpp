#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"
#include "DreamcastScreen.hpp"
#include "DreamcastTimer.hpp"
#include "DreamcastVibration.hpp"
#include "DreamcastMicrophone.hpp"
#include "DreamcastArGun.hpp"
#include "DreamcastKeyboard.hpp"
#include "DreamcastGun.hpp"
#include "DreamcastMouse.hpp"
#include "DreamcastExMedia.hpp"
#include "DreamcastCamera.hpp"
#include "EndpointTxSchedulerInterface.hpp"
#include "Transmitter.hpp"
#include "utils.h"

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
        //! @returns mask items not handled
        virtual uint32_t peripheralFactory(uint32_t functionCode)
        {
            mPeripherals.clear();

            peripheralFactoryCheck<DreamcastController>(functionCode);
            peripheralFactoryCheck<DreamcastStorage>(functionCode);
            peripheralFactoryCheck<DreamcastScreen>(functionCode);
            peripheralFactoryCheck<DreamcastTimer>(functionCode);
            peripheralFactoryCheck<DreamcastVibration>(functionCode);
            peripheralFactoryCheck<DreamcastMicrophone>(functionCode);
            peripheralFactoryCheck<DreamcastArGun>(functionCode);
            peripheralFactoryCheck<DreamcastKeyboard>(functionCode);
            peripheralFactoryCheck<DreamcastGun>(functionCode);
            peripheralFactoryCheck<DreamcastMouse>(functionCode);
            peripheralFactoryCheck<DreamcastExMedia>(functionCode);
            peripheralFactoryCheck<DreamcastCamera>(functionCode);

            // TODO: handle other peripherals here

            return functionCode;
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

        //! Adds a peripheral to peripheral vector
        //! @param[in,out] functionCode  Mask which contains function codes to check; returns mask
        //!                              minus peripheral that was added, if any
        template <class PeripheralClass>
        inline void peripheralFactoryCheck(uint32_t& functionCode)
        {
            if (functionCode & PeripheralClass::FUNCTION_CODE)
            {
                mPeripherals.push_back(std::make_shared<PeripheralClass>(mAddr, mEndpointTxScheduler, mPlayerData));
                functionCode &= ~PeripheralClass::FUNCTION_CODE;
            }
        }

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