#include "bsp/board.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"
#include "device/dcd.h"

#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#include "maple.pio.h"
#include "hardware/pio.h"

#include "MapleBus.hpp"
#include "DreamcastNode.hpp"

#include "UsbGamepad.h"
#include "usb_descriptors.h"
#include "usb_execution.h"

#define BUTTON_PIN 2

UsbGamepad player1UsbDevice(ITF_NUM_HID1, 0);
UsbControllerDevice* devices[] = {&player1UsbDevice};

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    DreamcastNode p1(14, 0, player1UsbDevice);

    while(true)
    {
        p1.task(time_us_64());
    }
}

void waitButtonPress()
{
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    // Wait for button press
    while (gpio_get(BUTTON_PIN))
    {
        sleep_ms(25);
    }

    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);

    // Wait for button release
    while (!gpio_get(BUTTON_PIN))
    {
        sleep_ms(25);
    }

    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);
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


