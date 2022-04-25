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
            if (words[0] & 0x00000100)
            {
                // memory unit - get its info
                busP1.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x01, NULL, 0);
                success = busP1.read(words, len, DEFAULT_SYSTICK_READ_WAIT_US, 2000);
                if (success)
                {
                    // Write a little VMU icon
                    uint32_t words1[] = {
                        0x00000004, 0x00000000, 0x0000FFFF, 0x00000003, 0x8001C000, 0x00060000, 0x6000000C,
                        0x00003000, 0x0008000C, 0x10000009, 0xDC0C1000, 0x0009DC3F, 0x10000009, 0xDC3F1000,
                        0x0008000C, 0x10000008, 0x360C1000, 0x00083600, 0x10000008, 0x00001000, 0x00080000,
                        0x10000008, 0x7FFE1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008,
                        0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000,
                        0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF, 0x10000008, 0xFFFF1000, 0x0008FFFF,
                        0x10000008, 0x7FFE1000, 0x000C0000, 0x30000006, 0x00006000, 0x00038001, 0xC0000000,
                        0xFFFF0000
                    };
                    busP1.write(MapleBus::COMMAND_BLOCK_WRITE, 0x01, words1, sizeof(words1) / sizeof(words1[0]));
                }
            }
        }

        sleep_ms(1000);
    }
}


