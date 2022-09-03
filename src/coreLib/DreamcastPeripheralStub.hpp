#pragma once

#include "DreamcastPeripheral.hpp"
#include "MapleBusInterface.hpp"
#include "PlayerData.hpp"

class DreamcastPeripheralStub : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] bus  The bus this controller is connected to
        //! @param[in] playerData  Data tied to player which controls this controller
        DreamcastPeripheralStub(uint8_t addr, MapleBusInterface& bus, PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastPeripheralStub();

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
        //! Time which the next controller state poll will occur
        uint64_t mNextCheckTime;
        //! True iff the controller is waiting for data
        bool mWaitingForData;
        //! Number of consecutive times no data was received
        uint32_t mNoDataCount;
};
