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
        // Request info from controller
        busP1.write(MapleBus::COMMAND_DEVICE_INFO_REQUEST, 0x20, NULL, 0);

        sleep_ms(100);
    }
}


