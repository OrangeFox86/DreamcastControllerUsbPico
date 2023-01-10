#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "configuration.h"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"

#include "DreamcastMainPeripheral.hpp"
#include "DreamcastController.hpp"

#include <memory>
#include <algorithm>

#define MAX_DEVICES 4

const uint8_t MAPLE_HOST_ADDRESSES[MAX_DEVICES] = {0x00, 0x40, 0x80, 0xC0};

// First Core Process
// The first core is in charge of initialization and USB communication
int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

#if SHOW_DEBUG_MESSAGES
    stdio_uart_init();
    stdio_usb_init();
#endif

    std::shared_ptr<MapleBusInterface> bus = create_maple_bus(P1_BUS_START_PIN);
    client::DreamcastMainPeripheral mainPeripheral(
        0x20,
        0xFF,
        0x00,
        "Dreamcast Controller",
        "Produced By or Under License From SEGA ENTERPRISES,LTD.",
        "Version 1.010,1998/09/28,315-6211-AB   ,Analog Module : The 4th Edition.5/8  +DF",
        43.0,
        50.0);
    mainPeripheral.addFunction(std::make_shared<client::DreamcastController>());

    while(true)
    {
        if (bus->startRead(1000000))
        {
            MapleBusInterface::Status status;
            do
            {
                status = bus->processEvents(time_us_64());
            } while (status.phase == MapleBusInterface::Phase::WAITING_FOR_READ_START
                    || status.phase == MapleBusInterface::Phase::READ_IN_PROGRESS);

            if (status.phase == MapleBusInterface::Phase::READ_COMPLETE)
            {
                MaplePacket packetOut;
                if (mainPeripheral.dispensePacket(status.readBuffer, status.readBufferLen, packetOut))
                {
                    if (bus->write(packetOut, false))
                    {
                        do
                        {
                            status = bus->processEvents(time_us_64());
                        } while (status.phase == MapleBusInterface::Phase::WRITE_IN_PROGRESS);
                    }
                }
            }
            else
            {
                mainPeripheral.reset();
            }
        }
    }
}

#endif
