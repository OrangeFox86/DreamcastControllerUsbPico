#ifndef DREAMCAST_CONTROLLER_USB_PICO_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "configuration.h"

#include "DreamcastMainNode.hpp"
#include "PlayerData.hpp"
#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"

#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/Usb/usb_interface.hpp"

#include <memory>
#include <algorithm>

#define MAX_DEVICES 4

const uint8_t MAPLE_HOST_ADDRESSES[MAX_DEVICES] = {0x00, 0x40, 0x80, 0xC0};

// Second Core Process
// The second core is in charge of handling communication with Dreamcast peripherals
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    uint32_t numUsbControllers = get_num_usb_controllers();
    uint32_t numDevices = std::min(numUsbControllers, (uint32_t)MAX_DEVICES);

    uint32_t maplePins[MAX_DEVICES] = {
        P1_BUS_START_PIN, P2_BUS_START_PIN, P3_BUS_START_PIN, P4_BUS_START_PIN
    };
    CriticalSectionMutex screenMutexes[numDevices];
    std::shared_ptr<ScreenData> screenData[numDevices];
    std::shared_ptr<PlayerData> playerData[numDevices];
    DreamcastControllerObserver** observers = get_usb_controller_observers();
    std::shared_ptr<MapleBusInterface> buses[numDevices];
    std::shared_ptr<DreamcastMainNode> dreamcastMainNodes[numDevices];
    Clock clock;
    for (uint32_t i = 0; i < numDevices; ++i)
    {
        screenData[i] = std::make_shared<ScreenData>(screenMutexes[i]);
        playerData[i] = std::make_shared<PlayerData>(i,
                                                     *(observers[i]),
                                                     *screenData[i],
                                                     clock,
                                                     usb_msc_get_file_system());
        buses[i] = create_maple_bus(maplePins[i], MAPLE_HOST_ADDRESSES[i]);
        dreamcastMainNodes[i] = std::make_shared<DreamcastMainNode>(
            *buses[i],
            *playerData[i],
            std::make_shared<PrioritizedTxScheduler>(DreamcastMainNode::MAX_PRIORITY));
    }

    while(true)
    {
        for (uint32_t i = 0; i < numDevices; ++i)
        {
            // Worst execution duration of below is ~350 us at 133 MHz when debug print is disabled
            dreamcastMainNodes[i]->task(time_us_64());
        }
    }
}

// First Core Process
// The first core is in charge of initialization and USB communication
int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

#if SHOW_DEBUG_MESSAGES
    stdio_uart_init();
#endif

    // complete read operations! Use a regular mutex instead.
    Mutex fileMutex;
    usb_msc_set_mutex(&fileMutex);

    multicore_launch_core1(core1);

    usb_init();

    while(true)
    {
        usb_task();
    }
}

#endif
