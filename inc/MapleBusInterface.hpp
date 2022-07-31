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

        //! Retrieves the last valid set of data read.
        //! @param[out] len  The number of words received
        //! @param[out] newData  Set to true iff new data was received since the last call
        //! @returns a pointer to the bytes read.
        //! @warning this call should be serialized with calls to write() as those calls may change
        //!          the data in the underlying buffer which is returned.
        virtual const uint32_t* getReadData(uint32_t& len, bool& newData) = 0;

        //! Processes timing events for the current time.
        //! @param[in] currentTimeUs  The current time to process for (0 to internally get time)
        virtual void processEvents(uint64_t currentTimeUs=0) = 0;

        //! @returns true iff the bus is currently busy reading or writing.
        virtual bool isBusy() = 0;
};

#endif // __MAPLE_BUS_INTERFACE_H__
