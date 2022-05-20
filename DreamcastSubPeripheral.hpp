#pragma once

#include <stdint.h>
#include "DreamcastPeripheral.hpp"

class DreamcastSubPeripheral : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] bus  The bus that this sub-peripheral is connected to
        DreamcastSubPeripheral(MapleBus& bus, uint8_t addr) : DreamcastPeripheral(bus, addr) {}

        //! Virtual destructor
        virtual ~DreamcastSubPeripheral() {}

        //! The task that a main peripheral yields control to after this peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @param[in] maxDurationUs  The sub peripheral must accomplish its next task within this time
        virtual void task(uint64_t currentTimeUs, uint64_t maxDurationUs) = 0;
};
