#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "configuration.h"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"
#include "NonVolatilePicoSystemMemory.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/Usb/host_usb_interface.hpp"
#include "GamepadHost.hpp"

#include "DreamcastMainPeripheral.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"
#include "DreamcastVibration.hpp"

#include "led.hpp"

#include <memory>
#include <algorithm>

void hid_set_controller(client::DreamcastController* ctrlr);

std::shared_ptr<NonVolatilePicoSystemMemory> mem =
    std::make_shared<NonVolatilePicoSystemMemory>(
        PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES,
        client::DreamcastStorage::MEMORY_SIZE_BYTES);

// Second Core Process
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    usb_init();

    printf("Starting USB host\n");

    while (true)
    {
        usb_task(time_us_64());
        mem->process();
    }
}

// First Core Process
void core0()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Create the bus for client-mode operation
    std::shared_ptr<MapleBusInterface> bus = create_maple_bus(P1_BUS_START_PIN);

    // Main peripheral (address of 0x20) with 1 function: controller
    client::DreamcastMainPeripheral mainPeripheral(
        bus,
        0x20,
        0xFF,
        0x00,
        "Dreamcast Controller",
        "Produced By or Under License From SEGA ENTERPRISES,LTD.",
        "Version 1.010,1998/09/28,315-6211-AB   ,Analog Module : The 4th Edition.5/8  +DF",
        43.0,
        50.0);
    std::shared_ptr<client::DreamcastController> controller =
        std::make_shared<client::DreamcastController>();
    set_gamepad_host(controller.get());
    mainPeripheral.addFunction(controller);

    // First sub peripheral (address of 0x01) with 1 function: memory
    std::shared_ptr<client::DreamcastPeripheral> subPeripheral1 =
        std::make_shared<client::DreamcastPeripheral>(
            0x01,
            0xFF,
            0x00,
            "Memory",
            "Produced By or Under License From SEGA ENTERPRISES,LTD.",
            "Version 1.005,1999/04/15,315-6208-03,SEGA Visual Memory System BIOS",
            12.4,
            13.0);
    std::shared_ptr<client::DreamcastStorage> dremcastStorage =
        std::make_shared<client::DreamcastStorage>(mem, 0);
    subPeripheral1->addFunction(dremcastStorage);
    mainPeripheral.addSubPeripheral(subPeripheral1);

    // Second sub peripheral (address of 0x02) with 1 function: vibration
    std::shared_ptr<client::DreamcastPeripheral> subPeripheral2 =
        std::make_shared<client::DreamcastPeripheral>(
            0x02,
            0xFF,
            0x00,
            "Puru Puru Pack",
            "Produced By or Under License From SEGA ENTERPRISES,LTD.",
            "Version 1.000,1998/11/10,315-6211-AH   ,Vibration Motor:1 , Fm:4 - 30Hz ,Pow:7",
            20.0,
            160.0);
    std::shared_ptr<client::DreamcastVibration> dreamcastVibration =
        std::make_shared<client::DreamcastVibration>();
    dreamcastVibration->setObserver(get_usb_vibration_observer());
    subPeripheral2->addFunction(dreamcastVibration);
    mainPeripheral.addSubPeripheral(subPeripheral2);

    multicore_launch_core1(core1);

    while(true)
    {
        mainPeripheral.task(time_us_64());
        led_task(mem->getLastActivityTime());
    }
}

int main()
{
    led_init();
    core0();
    return 0;
}

#endif
