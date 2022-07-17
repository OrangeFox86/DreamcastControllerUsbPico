#ifndef __MAPLE_BUS_INTERFACE_H__
#define __MAPLE_BUS_INTERFACE_H__

#include <stdint.h>
#include "configuration.h"
#include "utils.h"

//! Maple Bus interface class
class MapleBusInterface
{
    public:
        //! Virtual desturctor
        virtual ~MapleBusInterface() {}

        //! Writes a command to the Maple Bus where the sender address is what was given in the
        //! constructor.
        //! @param[in] command  The command byte - should be a value in Command enumeration
        //! @param[in] recipientAddr  The address of the device receiving this command
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(uint8_t command,
                           uint8_t recipientAddr,
                           const uint32_t* payload,
                           uint8_t len,
                           bool expectResponse,
                           uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US) = 0;

        //! Writes a command with the given custom frame word. The internal sender address is
        //! ignored and instead the given frame word is sent verbatim.
        //! @param[in] frameWord  The first word to put out on the bus
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(uint32_t frameWord,
                           const uint32_t* payload,
                           uint8_t len,
                           bool expectResponse,
                           uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US) = 0;

        //! Writes a command with the given words. The internal sender address is ignored and
        //! instead the given words are sent verbatim.
        //! @param[in] words  All words to send
        //! @param[in] len  Number of words in words (must be at least 1)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(const uint32_t* words,
                           uint8_t len,
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
