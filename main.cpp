#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "MapleBus.hpp"
#include "configuration.h"

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);
    while(true);
}

int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);
    multicore_launch_core1(core1);

    // The one bus on pins 14 and 15
    MapleBus busP1(14, 15, 0);

    while(true)
    {
        uint32_t data[] = {0x03020100,
                           0x07060504,
                           0x0B0A0908,
                           0x0F0E0D0C,
                           0x13121110,
                           0x17161514,
                           0x1B1A1918,
                           0x1F1E1D1C,
                           0x23222120,
                           0x27262524,
                           0x2B2A2928,
                           0x2F2E2D2C};
        busP1.write(0xFF, 0x55, data, sizeof(data) / sizeof(data[1]));

        sleep_ms(100);
    }
}


