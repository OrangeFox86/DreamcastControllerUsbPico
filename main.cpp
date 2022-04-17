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
// This should be able to handle a minumum of 31 x 32-bit words plus crc byte
volatile uint32_t states[2048];
volatile uint32_t* volatile currentState = states;
volatile uint32_t numStates = 0;

void isr_systick(void)
{
    if (numStates > 0)
    {
        gpio_put_all(*currentState++);
        --numStates;
    }
    
    if (numStates == 0)
    {
        // All done
        systick_hw->csr = 0;
    }
}

static inline void reset(void)
{
    numStates = 0;
    currentState = states;
}

static inline volatile uint32_t* flush(volatile uint32_t* ptr)
{
    if (ptr != NULL)
    {
        numStates = ptr - currentState;
    }

    if (numStates > 0)
    {
        systick_hw->csr |= (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_TICKINT_MASK | SYST_CSR_ENABLE_MASK);
        systick_hw->cvr = 0;
        while (systick_hw->csr & SYST_CSR_ENABLE_MASK);
    }
    else
    {
        systick_hw->csr = 0;
    }

    reset();

    return currentState;
}

void write(uint8_t* bytes, uint32_t len)
{
    volatile uint32_t* nextState = flush(NULL);

    // Ensure it's at neutral for a few cycles
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);

    // Start
    *nextState++ = SDCKB_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKB_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKB_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKB_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKB_MASK;
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);

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
            if (prepState != *(nextState - 1))
            {
                *nextState++ = prepState;
            }

            // Clock it
           *nextState++ = clockState;
        }

        // flush if buffer is about to overflow
        if ((nextState + 16) > (states + ((sizeof(states) / sizeof(states[0])) - 1)))
        {
            nextState = flush(nextState);
        }
    }

    // End
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);
    *nextState++ = SDCKA_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKA_MASK;
    *nextState++ = 0;
    *nextState++ = SDCKA_MASK;
    *nextState++ = (SDCKA_MASK | SDCKB_MASK);

    // Write and block until done
    flush(nextState);
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
