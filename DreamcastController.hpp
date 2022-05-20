#pragma once

#include "DreamcastMainPeripheral.hpp"
#include "MapleBus.hpp"
#include "UsbGamepad.h"

class DreamcastController : public DreamcastMainPeripheral
{
    public:
        struct ControllerCondition
        {
            uint8_t l; // 255: fully pressed

            uint8_t r; // 255: fully pressed

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

            uint8_t rAnalogUD; // Always 128

            uint8_t rAnalogLR; // Always 128

            uint8_t lAnalogUD; // 0: up; 128: neutral; 255: down

            uint8_t lAnalogLR; // 0: left; 128: neutral; 255: right
        } __attribute__ ((packed));

    public:
        //! Constructor
        //! @param[in] bus  The bus this controller is connected to
        DreamcastController(MapleBus& bus, uint32_t playerIndex, UsbGamepad& gamepad);

        virtual ~DreamcastController();

        //! Handles incoming data destined for this device
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

        //! Inherited from DreamcastMainPeripheral
        virtual bool task(uint64_t currentTimeUs) final;

    private:
        static const uint32_t NO_DATA_DISCONNECT_COUNT = 5;
        static const uint32_t US_PER_CHECK = 16000;
        uint32_t mPlayerIndex;
        UsbGamepad& mGamepad;
        uint64_t mNextCheckTime;
        bool mWaitingForData;
        uint32_t mNoDataCount;
};
