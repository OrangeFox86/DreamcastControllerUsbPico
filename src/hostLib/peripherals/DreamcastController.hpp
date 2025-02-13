// MIT License
//
// Copyright (c) 2022-2025 James Smith of OrangeFox86
// https://github.com/OrangeFox86/DreamcastControllerUsbPico
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "DreamcastPeripheral.hpp"
#include "hal/Usb/DreamcastControllerObserver.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast controller peripheral
class DreamcastController : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this controller
        DreamcastController(uint8_t addr,
                            uint32_t fd,
                            std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                            PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastController();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastPeripheral
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        inline uint32_t getFunctionCode() override final
        {
            return FUNCTION_CODE;
        }

    public:
        //! Function code for controller
        static const uint32_t FUNCTION_CODE = DEVICE_FN_CONTROLLER;

    private:
        //! Time between each controller state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! The gamepad to write button presses to
        DreamcastControllerObserver& mGamepad;
        //! True iff the controller is waiting for data
        bool mWaitingForData;
        //! Initialized to true and set to false in task()
        bool mFirstTask;
        //! ID of the get condition transmission
        uint32_t mConditionTxId;
};
