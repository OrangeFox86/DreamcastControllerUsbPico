#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#include "maple.pio.h"
#include "hardware/pio.h"

#include "MapleBus.hpp"

#define BUTTON_PIN 2

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    while(true);
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
    multicore_launch_core1(core1);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir_out_masked(1<<PICO_DEFAULT_LED_PIN);
    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);

    gpio_init(BUTTON_PIN);
    gpio_set_dir_in_masked(1<<BUTTON_PIN);
    gpio_set_pulls(BUTTON_PIN, true, false);

    // Used for debugging
    gpio_init(13);
    gpio_set_dir_out_masked(1<<13);

    MapleBus p1(14, 0x00);

    while(true)
    {
        waitButtonPress();
        p1.task();
        if (p1.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x20, NULL, 0, true))
        {
            for (uint i = 0; i < 5; ++i)
            {
                sleep_ms(100);
                gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);
            }
            gpio_put(PICO_DEFAULT_LED_PIN, false);
        }
        else
        {
            gpio_put(PICO_DEFAULT_LED_PIN, true);
            sleep_ms(2000);
            gpio_put(PICO_DEFAULT_LED_PIN, false);
        }
    }
}


