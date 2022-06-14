#pragma once

#include "DreamcastPeripheral.hpp"
#include "MapleBus.hpp"
#include "UsbGamepad.h"

class DreamcastController : public DreamcastPeripheral
{
    public:
        struct ControllerCondition
        {
            uint8_t l; //!< 0: fully released; 255: fully pressed

            uint8_t r; //!< 0: fully released; 255: fully pressed

            // Digital bits:
            // 0: pressed
            // 1: released
            unsigned none1:1;
            unsigned y:1;
            unsigned x:1;
            unsigned none2:5;

            unsigned none3:1;
            unsigned b:1;
            unsigned a:1;
            unsigned start:1;
            unsigned up:1;
            unsigned down:1;
            unsigned left:1;
            unsigned right:1;

            uint8_t rAnalogUD; //!< Always 128

            uint8_t rAnalogLR; //!< Always 128

            uint8_t lAnalogUD; //!< 0: up; 128: neutral; 255: down

            uint8_t lAnalogLR; //!< 0: left; 128: neutral; 255: right
        } __attribute__ ((packed));

    public:
        //! Constructor
        //! @param[in] bus  The bus this controller is connected to
        //! @param[in] playerIndex  Player index of this controller [0,3]
        //! @param[in] gamepad  The gamepad to write button presses to
        DreamcastController(uint8_t addr, MapleBus& bus, uint32_t playerIndex, UsbGamepad& gamepad);

        //! Virtual destructor
        virtual ~DreamcastController();

        //! Inherited from DreamcastPeripheral
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

        //! Inherited from DreamcastPeripheral
        virtual bool task(uint64_t currentTimeUs) final;

    private:
        //! Number of times failed communication occurs before determining that the controller is
        //! disconnected
        static const uint32_t NO_DATA_DISCONNECT_COUNT = 5;
        //! Time between each controller state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! The gamepad to write button presses to
        UsbGamepad& mGamepad;
        //! Time which the next controller state poll will occur
        uint64_t mNextCheckTime;
        //! True iff the controller is waiting for data
        bool mWaitingForData;
        //! Number of consecutive times no data was received
        uint32_t mNoDataCount;
};
