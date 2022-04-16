#include "pico/stdlib.h"
#include "hardware/structs/systick.h"

#define CPU_FREQ_MHZ 133
#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

#define CLOCK_PERIOD_NS 320
#define CPU_TICKS_PER_PERIOD (int)(CLOCK_PERIOD_NS * CPU_FREQ_MHZ / 1000.0 + 0.5)

#define SYST_CSR_CLKSOURCE_MASK 0x00000004 // 1 for processor; 0 for external
#define SYST_CSR_TICKINT_MASK   0x00000002 // 1 enables isr_systick; 0 disables
#define SYST_CSR_ENABLE_MASK    0x00000001 // 1 enables counter; 0 disables

#define A_IS_CLOCK_MASK 0xAA

const uint SDCKA_PIN = 14;
const uint SDCKB_PIN = 15;

const uint SDCKA_MASK = (1 << SDCKA_PIN);
const uint SDCKB_MASK = (1 << SDCKB_PIN);

// The size of this doesn't represent bits but rather max number of state changes
uint32_t states[8192];
uint32_t* currentState = states;
uint32_t numStates = 0;

void isr_systick(void)
{
    gpio_put_all(*currentState++);
    
    if (--numStates == 0)
    {
        // All done
        systick_hw->csr = 0;
    }
}

void write(uint8_t* bytes, uint32_t len)
{
    // Make sure tick is disabled
    systick_hw->csr = 0;

    // Reset
    numStates = 0;
    currentState = states;

    // Ensure it's at neutral for a few cycles
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);

    // Start
    *currentState++ = SDCKB_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKB_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKB_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKB_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKB_MASK;
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);

    // Data
    for (uint32_t i = 0; i < len; ++i)
    {
        uint8_t b = bytes[i];
        for (uint8_t mask = 0x80; mask > 0; mask = mask >> 1)
        {
            // Prepare the clock and data lines before clocking it
            uint32_t prepState = 0;
            uint32_t clockState = 0;
            if (mask & A_IS_CLOCK_MASK)
            {
                // A is clock and B is data
                if (b & mask)
                {
                    clockState = SDCKB_MASK;
                }
                prepState = SDCKA_MASK | clockState;
            }
            else
            {
                // B is clock and A is data
                if (b & mask)
                {
                    clockState = SDCKA_MASK;
                }
                prepState = SDCKB_MASK | clockState;
            }
            
            // Applying preperation state is optional if we're already there
            if (prepState != *(currentState - 1))
            {
                *currentState++ = prepState;
            }

            // Clock it
           *currentState++ = clockState;
        }
    }

    // End
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);
    *currentState++ = SDCKA_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKA_MASK;
    *currentState++ = 0;
    *currentState++ = SDCKA_MASK;
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);
    *currentState++ = (SDCKA_MASK | SDCKB_MASK);

    // Finish up
    numStates = currentState - states - 1;
    currentState = states;
    systick_hw->csr |= (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_TICKINT_MASK | SYST_CSR_ENABLE_MASK);
    systick_hw->cvr = 0;

    // Block until complete
    while (systick_hw->csr & SYST_CSR_ENABLE_MASK);
}

int main() 
{
    gpio_init_mask(SDCKA_MASK | SDCKB_MASK);
    gpio_set_dir_out_masked(SDCKA_MASK | SDCKB_MASK);
    gpio_put_all(SDCKA_MASK | SDCKB_MASK);
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    systick_hw->rvr = CPU_TICKS_PER_PERIOD - 1;

    uint8_t data[] = {0x00, 0xAA, 0x55, 0xFF, 0x00};
    write(data, sizeof(data));

    uint8_t data2[] = {0x00, 0x55, 0xFF, 0xAA, 0x00};
    write(data2, sizeof(data2));

    while(true);
}
