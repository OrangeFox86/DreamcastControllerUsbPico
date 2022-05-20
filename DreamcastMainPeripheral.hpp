#pragma once

#include <stdint.h>
#include <memory>
#include "MapleBus.hpp"
#include "DreamcastPeripheral.hpp"
#include "DreamcastSubPeripheral.hpp"

class DreamcastMainPeripheral : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] bus  The bus that this main peripheral is connected to
        DreamcastMainPeripheral(MapleBus& bus) :
            DreamcastPeripheral(bus, MAIN_PERIPHERAL_ADDR_MASK)
        {}

        //! Virtual destructor
        virtual ~DreamcastMainPeripheral() {}

        //! Called when the given sub peripheral is about to be removed
        //! @param[in] idx  The index of the sub peripheral being removed
        virtual void removingSubPeripheral(uint8_t idx)
        {}

        //! Called when a new sub peripheral is detected - the child must request device info and
        //! eventually add this sub peripheral
        //! @param[in] idx  The index of the detected sub peripheral
        virtual void newSubPeripheralDetected(uint8_t idx) = 0;

        //! Inherited from DreamcastPeripheral
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) = 0;

        //! Called by DreamcastNode to handle incoming data
        //! @param[in] len  Number of words in payload
        //! @param[in] senderAddress  Address of the peripheral that sent this data
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t senderAddr,
                                uint8_t cmd,
                                const uint32_t *payload)
        {
            if (senderAddr & mAddr)
            {
                // Use the sender address to determine if a sub-peripheral was added or removed
                uint8_t mask = SUB_PERIPHERAL_ADDR_START_MASK;
                for (int32_t i = 0; i < MAX_SUB_PERIPHERALS; ++i, mask<<=1)
                {
                    if (!mSubPeripherals[i] && ((senderAddr & mask) > 0))
                    {
                        newSubPeripheralDetected(i);
                    }
                    else if (mSubPeripherals[i] && ((senderAddr & mask) == 0))
                    {
                        removingSubPeripheral(i);
                        mSubPeripherals[i].reset();
                    }
                }

                // Have the device handle the data
                return handleData(len, cmd, payload);
            }
            else
            {
                int32_t idx = subPeripheralIndex(senderAddr);
                if (idx >= 0 && idx < MAX_SUB_PERIPHERALS)
                {
                    if (mSubPeripherals[idx])
                    {
                        return mSubPeripherals[idx]->handleData(len, cmd, payload);
                    }
                }
            }

            return false;
        }

        //! The task that DreamcastNode yields control to after this peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @returns true if the device is still connected or false if device disconnected
        virtual bool task(uint64_t currentTimeUs) = 0;

    protected:
        //! Yields task to sub-peripherals
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @param[in] maxDurationUs  The sub peripheral must accomplish its next task within this time
        inline void yieldTask(uint64_t currentTimeUs, uint64_t maxDurationUs)
        {
            for (int32_t i = 0; i < MAX_SUB_PERIPHERALS && !mBus.isBusy(); ++i)
            {
                if (mSubPeripherals[i])
                {
                    mSubPeripherals[i]->task(currentTimeUs, maxDurationUs);
                }
            }
        }

    public:
    protected:
        //! All of the sub-peripherals attached to this main peripheral
        std::unique_ptr<DreamcastSubPeripheral> mSubPeripherals[MAX_SUB_PERIPHERALS];
};
