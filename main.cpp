#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "MapleBus.hpp"
#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#include "maple.pio.h"
#include "hardware/pio.h"

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

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &maple_out_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_maple_out_init(pio, sm, offset, CPU_FREQ_KHZ, MIN_CLOCK_PERIOD_NS, 14);
    pio_maple_out_start(pio, sm);

    sleep_ms(1000);

    uint32_t dat[] = {0x00010203,0x04050607,0x08090A0B,0x0D0E0F10,0x20304050,0x60708090,0xA0B0C0D0,0xE0F0F1F2,0xF3F4F5F6,0xF7F8F9FA,0xFBFCFDFE,0xFF001122,0x33445566};
    uint32_t crc = 0;
    // Condition data (flip byte order) and compute crc
    for (uint i = 0; i < sizeof(dat)/ sizeof(dat[0]); ++i)
    {
        uint32_t word = 0;
        const uint8_t* src = reinterpret_cast<uint8_t*>(&dat[i]);
        uint8_t* dst = reinterpret_cast<uint8_t*>(&word) + 3;
        for (uint j = 0; j < 4; ++j, ++src, --dst)
        {
            *dst = *src;
            crc ^= *src;
        }
        dat[i] = word;
    }
    // The crc byte needs to be shifted to the upper byte of the uint32
    crc = crc << 24;

    // Send to FIFO!
    // First word is how many bits are going out
    pio_sm_put_blocking(pio, sm, sizeof(dat) * 8 + 8);
    for (uint i = 0; i < sizeof(dat)/ sizeof(dat[0]); ++i)
    {
        pio_sm_put_blocking(pio, sm, dat[i]);
    }
    pio_sm_put_blocking(pio, sm, crc);

    gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);
}


