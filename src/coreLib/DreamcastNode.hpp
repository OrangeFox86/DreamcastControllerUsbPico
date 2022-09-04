#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"
#include "DreamcastScreen.hpp"
#include "EndpointTxSchedulerInterface.hpp"

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
        //! @param[in] packet  The packet received
        //! @param[in] tx  The transmission that triggered this data
        //! @returns true iff the data was handled
        virtual bool handleData(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) = 0;

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

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual inline void txSent(std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                (*iter)->txSent(tx);
            }
        }

        //! Called when transmission failed
        //! @param[in] writeFailed  Set to true iff TX failed because write failed
        //! @param[in] readFailed  Set to true iff TX failed because read failed
        //! @param[in] tx  The transmission that failed
        virtual inline void txFailed(bool writeFailed,
                                     bool readFailed,
                                     std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                (*iter)->txFailed(writeFailed, readFailed, tx);
            }
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

        //! Try to get peripherals to handle the given data
        //! @param[in] packet  The packet received
        //! @param[in] tx  The transmission that triggered this data
        //! @returns true iff the data was handled
        bool handlePeripheralData(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
        {
            bool handled = false;
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end() && !handled;
                 ++iter)
            {
                handled = (*iter)->handleData(packet, tx);
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
                mPeripherals.push_back(std::make_shared<DreamcastController>(mAddr, mEndpointTxScheduler, mPlayerData));
            }
            else if (functionCode & DEVICE_FN_LCD)
            {
                mPeripherals.push_back(std::make_shared<DreamcastScreen>(mAddr, mEndpointTxScheduler, mPlayerData));
            }
            // TODO: handle other peripherals here
            // TODO: add a stub peripheral if none were created
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