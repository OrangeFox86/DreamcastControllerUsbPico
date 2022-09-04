#pragma once

#include <stdint.h>
#include "PrioritizedTxScheduler.hpp"
#include "EndpointTxSchedulerInterface.hpp"

//! Base class for a connected Dreamcast peripheral
class DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address (mask bit)
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerIndex  Player index of this peripheral [0,3]
        DreamcastPeripheral(uint8_t addr,
                            std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                            uint32_t playerIndex) :
            mEndpointTxScheduler(scheduler), mPlayerIndex(playerIndex), mAddr(addr)
        {}

        //! Virtual destructor
        virtual ~DreamcastPeripheral()
        {}

        //! Called when a transmission is complete
        //! @param[in] packet  The packet received or nullptr if this was write only transmission
        //! @param[in] tx  The transmission that triggered this data
        //! @returns true iff the data was handled
        virtual bool txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx) = 0;

        //! @param[in] subPeripheralIndex  Sub peripheral index [0,4]
        //! @returns the sub peripheral mask for the given sub peripheral index
        static inline uint8_t subPeripheralMask(int32_t subPeripheralIndex)
        {
            return SUB_PERIPHERAL_ADDR_START_MASK << subPeripheralIndex;
        }

        //! @param[in] subPeripheralMask  Sub peripheral mask
        //! @returns the index of the first sub peripheral mask that was matched
        static inline int32_t subPeripheralIndex(uint8_t subPeripheralMask)
        {
            uint8_t mask = SUB_PERIPHERAL_ADDR_START_MASK;
            for (uint32_t i = 0; i < MAX_SUB_PERIPHERALS; ++i, mask<<=1)
            {
                if (subPeripheralMask & mask)
                {
                    return i;
                }
            }
            return -1;
        }

        //! Get recipient address for a peripheral with given player index and address
        //! @param[in] playerIndex  Player index of peripheral [0,3]
        //! @param[in] addr  Peripheral's address (mask bit)
        //! @returns recipient address
        static inline uint8_t getRecipientAddress(uint32_t playerIndex, uint8_t addr)
        {
            return (playerIndex << 6) | addr;
        }

        //! @returns recipient address for this peripheral
        inline uint8_t getRecipientAddress() { return getRecipientAddress(mPlayerIndex, mAddr); }

        //! The task that DreamcastNode yields control to after this peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        virtual void task(uint64_t currentTimeUs) = 0;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual inline void txSent(std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
        {}

        //! Called when transmission failed
        //! @param[in] writeFailed  Set to true iff TX failed because write failed
        //! @param[in] readFailed  Set to true iff TX failed because read failed
        //! @param[in] tx  The transmission that failed
        virtual inline void txFailed(bool writeFailed,
                                     bool readFailed,
                                     std::shared_ptr<const PrioritizedTxScheduler::Transmission> tx)
        {}

    public:
        //! The maximum number of sub peripherals that a main peripheral can handle
        static const uint32_t MAX_SUB_PERIPHERALS = 5;
        //! Main peripheral address mask
        static const uint8_t MAIN_PERIPHERAL_ADDR_MASK = 0x20;
        //! The first sub peripheral address
        static const uint8_t SUB_PERIPHERAL_ADDR_START_MASK = 0x01;

    protected:
        //! Keeps all scheduled transmissions for the bus this peripheral is connected to
        std::shared_ptr<EndpointTxSchedulerInterface> mEndpointTxScheduler;
        //! Player index of this peripheral [0,3]
        const uint32_t mPlayerIndex;
        //! Address of this device
        const uint8_t mAddr;
};
