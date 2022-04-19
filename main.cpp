#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "mapleBusClocking.hpp"
#include "MapleBus.hpp"
#include "configuration.h"

void core1()
{
    while(true);
}

int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);
    clocking_init();
    multicore_launch_core1(core1);

    // The one bus on pins 14 and 15
    MapleBus busP1(14, 15);

    while(true)
    {
        uint8_t data[] = {0x0C, 0xAA, 0x55, 0xFF,
                        0x00, 0x01, 0x02, 0x03,
                        0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B,
                        0x0C, 0x0D, 0x0E, 0x0F,
                        0x10, 0x11, 0x12, 0x13,
                        0x14, 0x15, 0x16, 0x17,
                        0x18, 0x19, 0x1A, 0x1B,
                        0x1C, 0x1D, 0x1E, 0x1F,
                        0x20, 0x21, 0x22, 0x23,
                        0x24, 0x25, 0x26, 0x27,
                        0x28, 0x29, 0x2A, 0x2B,
                        0x2C, 0x2D, 0x2E, 0x2F,
                        0x00};
        write(busP1, data, sizeof(data));

        uint8_t data2[] = {0x00, 0x55, 0xFF, 0xAA, 0x00};
        write(busP1, data2, sizeof(data2));
    }
}


