#pragma once

#include <stdint.h>

class DreamcastMainPeripheral
{
    public:
        //! Virtual destructor
        virtual ~DreamcastMainPeripheral() {}

        //! The task that the DreamcastNode yields control to after this main peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @returns true if the device is still connected or false if device disconnected
        virtual bool task(uint64_t currentTimeUs) = 0;
};
