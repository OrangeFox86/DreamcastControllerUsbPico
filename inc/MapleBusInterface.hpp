#ifndef __MAPLE_BUS_INTERFACE_H__
#define __MAPLE_BUS_INTERFACE_H__

#include <stdint.h>
#include "configuration.h"
#include "utils.h"
#include "MaplePacket.hpp"

//! Maple Bus interface class
class MapleBusInterface
{
    public:
        //! Status due to processing events (see MapleBusInterface::processEvents)
        struct Status
        {
            //! A pointer to the bytes read or nullptr if no new data available
            const uint32_t* readBuffer;
            //! The number of words received or 0 if no new data available
            uint32_t readBufferLen;
            //! Set to true iff a write failed since the last call
            bool writeFail;
            //! Set to true iff a read failed since the last call
            bool readFail;

            Status() :
                readBuffer(nullptr),
                readBufferLen(0),
                writeFail(false),
                readFail(false)
            {}
        };

    public:
        //! Virtual desturctor
        virtual ~MapleBusInterface() {}

        //! Writes a packet to the maple bus
        //! @param[in] packet  The packet to send (sender address will be overloaded)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(const MaplePacket& packet,
                           bool expectResponse,
                           uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US) = 0;

        //! Processes timing events for the current time. This should be called before any write
        //! call in order to check timeouts and clear out any used resources.
        //! @param[in] currentTimeUs  The current time to process for
        //! @returns updated status since last call
        virtual Status processEvents(uint64_t currentTimeUs) = 0;

        //! @returns true iff the bus is currently busy reading or writing.
        virtual bool isBusy() = 0;
};

#endif // __MAPLE_BUS_INTERFACE_H__
