#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "configuration.h"
#include "hardware/structs/systick.h"
#include "hardware/regs/m0plus.h"

#include "maple.pio.h"
#include "hardware/pio.h"

#define BUTTON_PIN 2

void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    while(true);
}

inline void swap_byte_order(uint32_t& dest, uint32_t source, uint8_t& crc)
{
    const uint8_t* src = reinterpret_cast<uint8_t*>(&source);
    uint8_t* dst = reinterpret_cast<uint8_t*>(&dest) + 3;
    for (uint j = 0; j < 4; ++j, ++src, --dst)
    {
        *dst = *src;
        crc ^= *src;
    }
}

bool write_maple_blocking(PIO pio, uint sm, uint32_t frameWord, uint32_t* words, uint8_t len)
{

    uint32_t buffer[258];
    uint8_t crc = 0;
    swap_byte_order(buffer[0], frameWord, crc);
    for (uint32_t i = 0; i < len; ++i)
    {
        swap_byte_order(buffer[i + 1], words[i], crc);
    }
    buffer[len + 1] = crc << 24;

    // Send to FIFO!
    // First word is how many bits are going out
    uint32_t numBits = (len * 4 + 5) * 8;
    pio_sm_put_blocking(pio, sm, numBits);
    for (int i = 0; i < len + 2; ++i)
    {
        pio_sm_put_blocking(pio, sm, buffer[i]);
    }


    return true;
}

bool write_maple_blocking(PIO pio, uint sm, uint8_t command, uint8_t recipientAddr, uint32_t* words, uint8_t len)
{
    uint32_t frameWord = (len) | (recipientAddr << 16) | (command << 24);
    return write_maple_blocking(pio, sm, frameWord, words, len);
}

bool write_maple_blocking(PIO pio, uint sm, uint32_t* words, uint8_t len)
{
    return write_maple_blocking(pio, sm, words[0], words + 1, len - 1);
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
    for (uint i = 0; i < 3; ++i)
    {
        sleep_ms(100);
        gpio_xor_mask(1<<PICO_DEFAULT_LED_PIN);
    }
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

    gpio_init(13);
    gpio_set_dir_out_masked(1<<13);

    PIO pioOut = pio0;
    uint offsetOut = pio_add_program(pioOut, &maple_out_program);
    uint smOut1 = pio_claim_unused_sm(pioOut, true);
    pio_sm_config configOut = pio_maple_out_get_config(offsetOut, CPU_FREQ_KHZ, MIN_CLOCK_PERIOD_NS, 14);
    pio_maple_out_pin_init(pioOut, smOut1, 14);
    gpio_xor_mask(1<<13);
    pio_maple_out_start(pioOut, smOut1, offsetOut, configOut);
    gpio_xor_mask(1<<13);

    while(true)
    {
        waitButtonPress();
        write_maple_blocking(pioOut, smOut1, 0x01, 0x20, NULL, 0);
    }
}


