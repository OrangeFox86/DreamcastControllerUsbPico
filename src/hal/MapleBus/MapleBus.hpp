#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include <memory>
#include "hal/MapleBus/MapleBusInterface.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/dma.h"
#include "configuration.h"
#include "utils.h"
#include "maple.pio.h"
#include "hardware/pio.h"

//! Handles communication over Maple Bus. This class is currently only setup to handle communication
//! from a host which initiates communication. This can easily be modified to handle communication
//! for a device, but that is not a use-case of this project.
//!
//! If this is ever modified to be a device, keep in mind that the maple_in state machine doesn't
//! sample the full end sequence. The application side should wait a sufficient amount of time
//! after bus goes neutral before responding in that case. Waiting for neutral bus is enough anyway.
//!
//! @warning this class is not "thread safe" - it should only be used by 1 core.
class MapleBus : public MapleBusInterface
{
    public:
        //! Maple Bus constructor
        //! @param[in] pinA  GPIO index for pin A. The very next GPIO will be designated as pin B.
        //! @param[in] senderAddr  The address of this device
        MapleBus(uint32_t pinA, uint8_t senderAddr);

        //! Writes a packet to the maple bus
        //! @param[in] packet  The packet to send (sender address will be overloaded)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @returns true iff the bus was "open" and send has started
        bool write(const MaplePacket& packet,
                   bool expectResponse);

        //! Called from a PIO ISR when read has completed for this sender.
        void readIsr();

        //! Called from a PIO ISR when write has completed for this sender.
        void writeIsr();

        //! Processes timing events for the current time. This should be called before any write
        //! call in order to check timeouts and clear out any used resources.
        //! @param[in] currentTimeUs  The current time to process for
        //! @returns updated status since last call
        Status processEvents(uint64_t currentTimeUs);

        //! @returns true iff the bus is currently busy reading or writing.
        inline bool isBusy() { return mCurrentPhase != Phase::IDLE; }

    private:
        //! Ensures that the bus is open and starts the write PIO state machine.
        bool writeInit();

        //! Swaps the endianness of a 32 bit word from the given source to the given destination
        //! while also computing a crc over the 4 bytes.
        //! @param[out] dest  destination word to write to
        //! @param[in] source  source word to read from
        //! @param[in,out] crc  crc word to start from and write to
        static inline void swapByteOrder(volatile uint32_t& dest, uint32_t source, uint8_t& crc)
        {
            const uint8_t* src = reinterpret_cast<uint8_t*>(&source);
            volatile uint8_t* dst = reinterpret_cast<volatile uint8_t*>(&dest) + 3;
            for (uint j = 0; j < 4; ++j, ++src, --dst)
            {
                *dst = *src;
                crc ^= *src;
            }
        }

        //! Initializes all interrupt service routines for all Maple Busses
        static void initIsrs();

    private:
        //! Pin A GPIO index for this bus
        const uint32_t mPinA;
        //! Pin B GPIO index for this bus
        const uint32_t mPinB;
        //! Pin A GPIO mask for this bus
        const uint32_t mMaskA;
        //! Pin B GPIO mask for this bus
        const uint32_t mMaskB;
        //! GPIO mask for all bits used by this bus
        const uint32_t mMaskAB;
        //! The address of this device
        const uint8_t mSenderAddr;
        //! The PIO state machine used for output by this bus
        const MapleOutStateMachine mSmOut;
        //! The PIO state machine index used for input by this bus
        const MapleInStateMachine mSmIn;
        //! The DMA channel used for writing by this bus
        const int mDmaWriteChannel;
        //! The DMA channel used for reading by this bus
        const int mDmaReadChannel;

        //! The output word buffer - 256 + 2 extra words for bit count and CRC
        volatile uint32_t mWriteBuffer[258];
        //! The input word buffer - 256 + 1 extra word for CRC
        volatile uint32_t mReadBuffer[257];
        //! Persistent storage for external use after processEvents call
        uint32_t mLastRead[256];
        //! Current phase of the state machine
        Phase mCurrentPhase;
        //! True if read should be started immediately after write has completed
        bool mExpectingResponse;
        //! The time at which the next timeout will occur
        volatile uint64_t mProcKillTime;
        //! The last time which number of received words changed
        uint64_t mLastReceivedWordTimeUs;
        //! The last sampled read word transfer count
        uint32_t mLastReadTransferCount;
};

std::shared_ptr<MapleBusInterface> create_maple_bus(uint32_t pinA, uint8_t senderAddr)
{
    return std::make_shared<MapleBus>(pinA, senderAddr);
}

#endif // __MAPLE_BUS_H__
