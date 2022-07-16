#pragma once

#include "DreamcastPeripheral.hpp"
#include "MapleBus.hpp"
#include "ScreenData.hpp"

class DreamcastScreen : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] bus  The bus this screen is connected to
        //! @param[in] playerData  Data tied to player which controls this screen
        DreamcastScreen(uint8_t addr, MapleBus& bus, PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastScreen();

        //! Inherited from DreamcastPeripheral
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) final;

        //! Inherited from DreamcastPeripheral
        virtual bool task(uint64_t currentTimeUs) final;

    private:
        //! Number of times failed communication occurs before determining that the screen is
        //! disconnected
        static const uint32_t NO_DATA_DISCONNECT_COUNT = 5;
        //! Time between each screen state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! Time which the next screen state poll will occur
        uint64_t mNextCheckTime;
        //! True iff the screen is waiting for data
        bool mWaitingForData;
        //! Number of consecutive times no data was received
        uint32_t mNoDataCount;

        ScreenData& mScreenData;
};
