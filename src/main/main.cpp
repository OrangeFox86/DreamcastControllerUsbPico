#ifndef DREAMCAST_CONTROLLER_USB_PICO_TEST

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"
#include "device/dcd.h"

#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#include "MapleBus.hpp"
#include "DreamcastNode.hpp"
#include "DreamcastMainNode.hpp"
#include "PlayerData.hpp"
#include "CriticalSectionMutex.hpp"

#include "UsbGamepad.h"
#include "UsbGamepadDreamcastControllerObserver.hpp"
#include "usb_descriptors.h"
#include "usb_execution.h"

#define BUTTON_PIN 2

#define MAPLE_HOST_ADDRESS 0x00
#define P1_BUS_START_PIN 14

UsbGamepad player1UsbDevice(ITF_NUM_HID1, 0);
UsbGamepadDreamcastControllerObserver player1Observer(player1UsbDevice);
UsbControllerInterface* devices[] = {&player1UsbDevice};
CriticalSectionMutex screenMutex;
ScreenData player1ScreenData(screenMutex);

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    PlayerData playerData = {0, player1Observer, player1ScreenData};
    MapleBus busP1(P1_BUS_START_PIN, MAPLE_HOST_ADDRESS);
    DreamcastMainNode p1(busP1, playerData);

    while(true)
    {
        p1.task(time_us_64());
    }
}

int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    board_init();

    multicore_launch_core1(core1);

    set_usb_devices(devices, 1);

    usb_init();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir_out_masked(1<<PICO_DEFAULT_LED_PIN);
    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);

    gpio_init(BUTTON_PIN);
    gpio_set_dir_in_masked(1<<BUTTON_PIN);
    gpio_set_pulls(BUTTON_PIN, true, false);

    // Used for debugging
    gpio_init(13);
    gpio_set_dir_out_masked(1<<13);

    while(true)
    {
        usb_task();
    }
}

#endif
