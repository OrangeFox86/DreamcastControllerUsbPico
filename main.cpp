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

enum Bank
{
    BANK_A = 0,
    BANK_B,

    BANK_COUNT
};

// The size of this doesn't represent bits but rather max number of state changes
#define ELEMENTS_PER_BANK 256

class DataBank
{
    public:
        DataBank() :
            mData(),
            mCurrent(mData),
            mEnd(mData),
            mCapacityEnd(mData + ELEMENTS_PER_BANK)
        {}

        inline void reset() volatile
        {
            mEnd = mData;
            mCurrent = mData;
        }

        inline void write(uint32_t datum) volatile
        {
            *mEnd++ = datum;
        }

        inline uint32_t read() volatile
        {
            return *mCurrent++;
        }

        inline int32_t remainingCapacity() volatile
        {
            return (mCapacityEnd - mEnd);
        }

        inline bool empty() volatile
        {
            return (mEnd == mCurrent);
        }

    private:
        volatile uint32_t mData[ELEMENTS_PER_BANK];
        volatile uint32_t* volatile mCurrent;
        volatile uint32_t* volatile mEnd;
        volatile uint32_t* volatile const mCapacityEnd;
};

volatile DataBank g_dataBanks[BANK_COUNT];
volatile DataBank* g_currentBank = &g_dataBanks[BANK_A];

extern "C" {
void isr_systick(void)
{
    if (!g_currentBank->empty())
    {
        gpio_put_all(g_currentBank->read());
    }
    
    if (g_currentBank->empty())
    {
        // All done
        systick_hw->csr = 0;
    }
}
}

static inline void reset_all(void)
{
    for (uint32_t i = BANK_A; i < BANK_COUNT; ++i)
    {
        g_dataBanks[i].reset();
    }
    g_currentBank = &g_dataBanks[BANK_A];
}

static inline void flush()
{
    if (!(systick_hw->csr & SYST_CSR_ENABLE_MASK))
    {
        systick_hw->csr |= (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_TICKINT_MASK | SYST_CSR_ENABLE_MASK);
        systick_hw->cvr = 0;
    }
    while (systick_hw->csr & SYST_CSR_ENABLE_MASK);
    g_currentBank->reset();
}

static inline Bank send(Bank bank)
{
    if (&g_dataBanks[bank] != g_currentBank)
    {
        if (!g_currentBank->empty())
        {
            flush();
        }
        g_currentBank = &g_dataBanks[bank];
    }

    if (!(systick_hw->csr & SYST_CSR_ENABLE_MASK))
    {
        systick_hw->csr |= (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_TICKINT_MASK | SYST_CSR_ENABLE_MASK);
        systick_hw->cvr = 0;
    }

    bank = static_cast<Bank>(static_cast<uint32_t>(bank) + 1);
    if (bank >= BANK_COUNT)
    {
        bank = BANK_A;
    }
    return bank;
}

void write(uint8_t* bytes, uint32_t len)
{
    reset_all();
    Bank bank = BANK_A;
    volatile DataBank* dataBank = &g_dataBanks[bank];

    // Ensure it's at neutral for a few cycles
    dataBank->write(SDCKA_MASK | SDCKB_MASK);
    dataBank->write(SDCKA_MASK | SDCKB_MASK);
    dataBank->write(SDCKA_MASK | SDCKB_MASK);

    // Start
    dataBank->write(SDCKB_MASK);
    dataBank->write(0);
    dataBank->write(SDCKB_MASK);
    dataBank->write(0);
    dataBank->write(SDCKB_MASK);
    dataBank->write(0);
    dataBank->write(SDCKB_MASK);
    dataBank->write(0);
    dataBank->write(SDCKB_MASK);
    dataBank->write(SDCKA_MASK | SDCKB_MASK);

    // Data
    uint32_t lastState = (SDCKA_MASK | SDCKB_MASK);
    for (uint32_t i = 0; i < len; ++i)
    {
        // flush if buffer could possibly overflow 
        if (dataBank->remainingCapacity() < 16)
        {
            bank = send(bank);
            dataBank = &g_dataBanks[bank];
            dataBank->reset();
        }

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
            if (prepState != lastState)
            {
                dataBank->write(prepState);
            }

            // Clock it
           dataBank->write(clockState);
           lastState = clockState;
        }
    }

    // flush if buffer could possibly overflow 
    if (dataBank->remainingCapacity() < 7)
    {
        bank = send(bank);
        dataBank = &g_dataBanks[bank];
    }

    // End
    dataBank->write(SDCKA_MASK | SDCKB_MASK);
    dataBank->write(SDCKA_MASK);
    dataBank->write(0);
    dataBank->write(SDCKA_MASK);
    dataBank->write(0);
    dataBank->write(SDCKA_MASK);
    dataBank->write(SDCKA_MASK | SDCKB_MASK);

    // Write and block until done
    send(bank);
    flush();
}

int main() 
{
    gpio_init_mask(SDCKA_MASK | SDCKB_MASK);
    gpio_set_dir_out_masked(SDCKA_MASK | SDCKB_MASK);
    gpio_put_all(SDCKA_MASK | SDCKB_MASK);
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    systick_hw->rvr = CPU_TICKS_PER_PERIOD - 1;

    gpio_put_all(0);
    sleep_us(1);
    gpio_put_all(SDCKA_MASK | SDCKB_MASK);

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
    write(data, sizeof(data));

    uint8_t data2[] = {0x00, 0x55, 0xFF, 0xAA, 0x00};
    write(data2, sizeof(data2));

    while(true);
}
