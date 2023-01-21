#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "configuration.h"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"
#include "VolatileSystemMemory.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"

#include "DreamcastMainPeripheral.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"

#include <memory>
#include <algorithm>

#define MAX_DEVICES 4

const uint8_t MAPLE_HOST_ADDRESSES[MAX_DEVICES] = {0x00, 0x40, 0x80, 0xC0};

// First Core Process
// The first core is in charge of initialization and USB communication
void core0()
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
    std::shared_ptr<client::DreamcastPeripheral> subPeripheral1 =
        std::make_shared<client::DreamcastPeripheral>(
            0x01,
            0xFF,
            0x00,
            "Memory",
            "Produced By or Under License From SEGA ENTERPRISES,LTD.",
            "Version 1.005,1999/04/15,315-6208-03,SEGA Visual Memory System BIOS Produced by IOS Produced",
            12.4,
            13.0);
    std::shared_ptr<VolatileSystemMemory> mem =
        std::make_shared<VolatileSystemMemory>(client::DreamcastStorage::MEMORY_SIZE_BYTES);
    std::shared_ptr<client::DreamcastStorage> dremcastStorage =
        std::make_shared<client::DreamcastStorage>(mem, 0);
    dremcastStorage->format();
    subPeripheral1->addFunction(dremcastStorage);
    mainPeripheral.addSubPeripheral(subPeripheral1);

    uint8_t lastSender = 0;
    MaplePacket packetOut;
    packetOut.reservePayload(256);
    MaplePacket packetIn;
    packetIn.reservePayload(256);
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
                bool writeIt = false;

                packetIn.set(status.readBuffer, status.readBufferLen);
                lastSender = packetIn.frame.senderAddr;

                if (packetIn.frame.command == COMMAND_RESPONSE_REQUEST_RESEND)
                {
                    // Write the previous packet
                    writeIt = true;
                }
                else
                {
                    packetOut.reset();
                    writeIt = mainPeripheral.dispensePacket(packetIn, packetOut);
                }

                if (!writeIt)
                {
                    packetOut.frame.command = COMMAND_RESPONSE_UNKNOWN_COMMAND;
                }

                if (packetOut.isValid())
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
            else if(status.phase == MapleBusInterface::Phase::READ_FAILED
                    && status.failureReason == MapleBusInterface::FailureReason::CRC_INVALID
                    && mainPeripheral.isConnected())
            {
                packetOut.reset();
                packetOut.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                packetOut.frame.recipientAddr = lastSender;
                packetOut.frame.senderAddr = mainPeripheral.getAddress();
                packetOut.updateFrameLength();
                if (bus->write(packetOut, false))
                {
                    do
                    {
                        status = bus->processEvents(time_us_64());
                    } while (status.phase == MapleBusInterface::Phase::WRITE_IN_PROGRESS);
                }
            }
            else
            {
                mainPeripheral.reset();
            }
        }
    }
}

int main()
{
    core0();
    return 0;
}

#endif
