#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "MapleBus.hpp"
#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    while(true);
}

int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);
    multicore_launch_core1(core1);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir_out_masked(1<<PICO_DEFAULT_LED_PIN);
    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);

    gpio_init(13);
    gpio_set_dir_out_masked(1<<13);

    // The one bus on pins 14 and 15
    MapleBus busP1(14, 15, 0);

    // Wait for steady state
    sleep_ms(100);

    while(true)
    {
        // Request info from controller
        busP1.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x20, NULL, 0);

        uint32_t words[256];
        uint32_t len = (sizeof(words) / sizeof(words[0]));
        bool success = busP1.read(words, len);

        if (success)
        {
            gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);
        }

        sleep_ms(1000);
    }
}


