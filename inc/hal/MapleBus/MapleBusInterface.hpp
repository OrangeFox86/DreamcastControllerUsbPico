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
        //! Enumerates the phase in the state machine
        enum class Phase
        {
            //! Initialized phase and phase after completion and events are processed
            IDLE,
            //! Write is currently in progress
            WRITE_IN_PROGRESS,
            //! Write has failed
            WRITE_FAILED,
            //! Write completed and no read was expected
            WRITE_COMPLETE,
            //! Write completed, waiting for start sequence from device
            WAITING_FOR_READ_START,
            //! Currently waiting for response
            READ_IN_PROGRESS,
            //! Read has failed
            READ_FAILED,
            //! Write and read cycle completed
            READ_COMPLETE,
            //! Initialized value
            INVALID
        };

        //! Status due to processing events (see MapleBusInterface::processEvents)
        struct Status
        {
            //! The phase of the state machine
            Phase phase;
            //! A pointer to the bytes read or nullptr if no new data available
            const uint32_t* readBuffer;
            //! The number of words received or 0 if no new data available
            uint32_t readBufferLen;

            Status() :
                phase(Phase::INVALID),
                readBuffer(nullptr),
                readBufferLen(0)
            {}
        };

    public:
        //! Virtual desturctor
        virtual ~MapleBusInterface() {}

        //! Writes a packet to the maple bus
        //! @param[in] packet  The packet to send (sender address will be overloaded)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(const MaplePacket& packet,
                           bool expectResponse) = 0;

        //! Processes timing events for the current time. This should be called before any write
        //! call in order to check timeouts and clear out any used resources.
        //! @param[in] currentTimeUs  The current time to process for
        //! @returns updated status since last call
        virtual Status processEvents(uint64_t currentTimeUs) = 0;

        //! @returns true iff the bus is currently busy reading or writing.
        virtual bool isBusy() = 0;
};

#endif // __MAPLE_BUS_INTERFACE_H__
