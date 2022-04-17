#include "pico/stdlib.h"
#include "hardware/structs/systick.h"

#include "mapleBusClocking.hpp"
#include "MapleBus.hpp"
#include "MapleStateBank.hpp"
#include "configuration.h"

// From datasheet
#define SYST_CSR_CLKSOURCE_MASK 0x00000004 // 1 for processor; 0 for external
#define SYST_CSR_TICKINT_MASK   0x00000002 // 1 enables isr_systick; 0 disables
#define SYST_CSR_ENABLE_MASK    0x00000001 // 1 enables counter; 0 disables

// When masking the current bit mask with this, anything other than 0 means A channel is clock
#define A_IS_CLOCK_MASK 0xAA

// Code is tightly coupled to "2" banks, so it makes no sense to have this be anyting but
#define NUMBER_OF_BANKS 2

#if (ELEMENTS_PER_BANK < 16)
   #error "ELEMENTS_PER_BANK must be set to at least 16"
#endif

// Note: I believe I can get away without volatile keywords because this buffer is never
//       read/written to by main code and ISR at the same time. Without volatile, this
//       speeds up packing time by about 15%.
MapleStateBank g_dataBanks[NUMBER_OF_BANKS];
MapleStateBank* g_currentBank = &g_dataBanks[0];
uint32_t g_nextWrite = 0;

static inline uint32_t is_isr_systick_enabled()
{
    return (systick_hw->csr & SYST_CSR_ENABLE_MASK);
}

static inline void enable_isr_systick()
{
    if (!is_isr_systick_enabled() && !g_currentBank->empty())
    {
        g_nextWrite = g_currentBank->readNext();
        systick_hw->csr = (SYST_CSR_CLKSOURCE_MASK | SYST_CSR_TICKINT_MASK | SYST_CSR_ENABLE_MASK);
        systick_hw->cvr = 0;
    }
}

static inline void disable_isr_systick()
{
    systick_hw->csr = 0;
}

extern "C"
{
void isr_systick(void)
{
    // Doing this send as very first operation makes for more uniform clocking
    gpio_put_all(g_nextWrite);

    // I played around with masking, but that would increase bit send time by 15 to 50 ns
    // Without masking, no other gpio is allowed besides the bus being written to
    // gpio_put_masked(g_currentBank->getSendMask(), g_nextWrite);

    if (g_currentBank->empty())
    {
        disable_isr_systick();
    }
    else
    {
        g_nextWrite = g_currentBank->readNext();
    }
}
}

static inline void reset_all(void)
{
    for (uint32_t i = 0; i < NUMBER_OF_BANKS; ++i)
    {
        g_dataBanks[i].reset();
    }
    g_currentBank = &g_dataBanks[0];
}

void flush()
{
    enable_isr_systick();

    while (is_isr_systick_enabled());

    disable_isr_systick();
    g_currentBank->reset();
}

uint32_t send(uint32_t bankIdx)
{
    // Switch to new bank if we aren't already there
    if (&g_dataBanks[bankIdx] != g_currentBank)
    {
        // Wait for current write to complete if it's in the middle of sending
        if (is_isr_systick_enabled())
        {
            while (is_isr_systick_enabled());
        }
        // Note: the above check is necessary before checking data within current bank - volatile is
        //       not used, so read at the same time ISR may be operating on it is not allowed!
        // Flush anything that is waiting to be sent in the current bank
        else if (!g_currentBank->empty())
        {
            flush();
        }

        // Switch over to new bank
        g_currentBank = &g_dataBanks[bankIdx];
    }

    // Ensure this bank is being sent
    enable_isr_systick();

    // Return the next bank index
    if (++bankIdx >= NUMBER_OF_BANKS)
    {
        bankIdx = 0;
    }
    return bankIdx;
}

void write(const MapleBus& mapleBus, uint8_t* bytes, uint32_t len)
{
    reset_all();
    mapleBus.writeInit();
    uint32_t bankIdx = 0;
    MapleStateBank* dataBank = &g_dataBanks[bankIdx];
    dataBank->setMapleBus(mapleBus);

    // Ensure it's at neutral for a few cycles
    dataBank->write(MapleStateBank::MASK_AB);
    dataBank->write(MapleStateBank::MASK_AB);
    dataBank->write(MapleStateBank::MASK_AB);

    // Start
    dataBank->write(MapleStateBank::MASK_B);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_B);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_B);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_B);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_B);
    dataBank->write(MapleStateBank::MASK_AB);

    // Data
    uint32_t lastState = (MapleStateBank::MASK_AB);
    for (uint32_t i = 0; i < len; ++i)
    {
        // flush if buffer could possibly overflow
        if (dataBank->remainingCapacity() < 16)
        {
            bankIdx = send(bankIdx);
            dataBank = &g_dataBanks[bankIdx];
            dataBank->reset();
            dataBank->setMapleBus(mapleBus);
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
                    clockState = MapleStateBank::MASK_B;
                }
                prepState = MapleStateBank::MASK_A | clockState;
            }
            else
            {
                // B is clock and A is data
                if (b & mask)
                {
                    clockState = MapleStateBank::MASK_A;
                }
                prepState = MapleStateBank::MASK_B | clockState;
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
        bankIdx = send(bankIdx);
        dataBank = &g_dataBanks[bankIdx];
    }

    // End
    dataBank->write(MapleStateBank::MASK_AB);
    dataBank->write(MapleStateBank::MASK_A);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_A);
    dataBank->write(0);
    dataBank->write(MapleStateBank::MASK_A);
    dataBank->write(MapleStateBank::MASK_AB);

    // Write and block until done
    send(bankIdx);
    flush();
    mapleBus.writeComplete();
}

void clocking_init()
{
    systick_hw->rvr = CPU_TICKS_PER_PERIOD - 1;
}
