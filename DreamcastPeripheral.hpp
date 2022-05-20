#pragma once

#include <stdint.h>
#include "MapleBus.hpp"

class DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] bus  The bus that this peripheral is connected to
        //! @param[in] addr  This peripheral's address (mask bit)
        DreamcastPeripheral(MapleBus& bus, uint8_t addr) : mBus(bus), mAddr(addr) {}

        //! Virtual destructor
        virtual ~DreamcastPeripheral() {}

        //! Handles incoming data destined for this device
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) = 0;

        //! @param[in] subPeripheralIndex  Sub peripheral index [0,4]
        //! @returns the sub peripheral mask for the given sub peripheral index
        inline uint8_t subPeripheralMask(int32_t subPeripheralIndex)
        {
            return SUB_PERIPHERAL_ADDR_START_MASK << subPeripheralIndex;
        }

        //! @param[in] subPeripheralMask  Sub peripheral mask
        //! @returns the index of the first sub peripheral mask that was matched
        inline int32_t subPeripheralIndex(uint8_t subPeripheralMask)
        {
            uint8_t mask = SUB_PERIPHERAL_ADDR_START_MASK;
            for (int32_t i = 0; i < MAX_SUB_PERIPHERALS; ++i, mask<<=1)
            {
                if (subPeripheralMask & i)
                {
                    return i;
                }
            }
            return -1;
        }

    public:
        //! The maximum number of sub peripherals that a main peripheral can handle
        static const int32_t MAX_SUB_PERIPHERALS = 5;
        //! Main peripheral address mask
        static const uint8_t MAIN_PERIPHERAL_ADDR_MASK = 0x20;
        //! The first sub peripheral address
        static const uint8_t SUB_PERIPHERAL_ADDR_START_MASK = 0x01;

    protected:
        //! The bus that this peripheral is connected to
        MapleBus& mBus;
        //! Address of this device
        uint8_t mAddr;
};
