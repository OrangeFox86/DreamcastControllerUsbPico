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

#define NUMBER_OF_BANKS 2

const uint32_t SDCKA_PIN = 14;
const uint32_t SDCKB_PIN = 15;

const uint32_t SDCKA_MASK = (1 << SDCKA_PIN);
const uint32_t SDCKB_MASK = (1 << SDCKB_PIN);

// The size of this doesn't represent bits but rather max number of state changes
#define ELEMENTS_PER_BANK 256

class MapleStateBank
{
    public:
        MapleStateBank() :
            mData(),
            mCurrent(mData),
            mEnd(mData),
            mCapacityEnd(mData + ELEMENTS_PER_BANK),
            mBitShift(0),
            mSendMask(MASK_AB)
        {}

        inline void reset()
        {
            mEnd = mData;
            mCurrent = mData;
        }

        inline void write(uint_fast8_t datum)
        {
            *mEnd++ = (datum << mBitShift);
        }

        inline uint_fast32_t readNext()
        {
            return *mCurrent++;
        }

        inline int_fast32_t remainingCapacity()
        {
            return (mCapacityEnd - mEnd);
        }

        inline bool empty()
        {
            return (mEnd == mCurrent);
        }

        inline void setOutPins(uint_fast32_t a, uint_fast32_t b)
        {
            assert((b - a) == 1);
            mBitShift = a;
            mSendMask = MASK_AB << mBitShift;
        }

        inline uint_fast32_t getSendMask()
        {
            return mSendMask;
        }

    public:
        static const uint_fast8_t MASK_A = 0x01;
        static const uint_fast8_t MASK_B = 0x02;
        static const uint_fast8_t MASK_AB = (MASK_A | MASK_B);

    private:
        // Note: I believe I can get away without volatile keywords because this buffer is never
        //       read/written to by main code and ISR at the same time. Without volatile, this
        //       speeds up packing time by about 15%.

        uint_fast32_t mData[ELEMENTS_PER_BANK];
        uint_fast32_t* mCurrent;
        uint_fast32_t* mEnd;
        uint_fast32_t* const mCapacityEnd;
        uint_fast32_t mBitShift;
        uint_fast32_t mSendMask;
};

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

extern "C" {
void isr_systick(void)
{
    // Doing this send as very first operation makes for more uniform clocking
    gpio_put_all(g_nextWrite);

    // I played around with masking, but that would increase bit send time by 16 ns
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

static inline void flush()
{
    enable_isr_systick();

    while (is_isr_systick_enabled());

    disable_isr_systick();
    g_currentBank->reset();
}

static inline uint32_t send(uint32_t bankIdx)
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

void write(uint8_t* bytes, uint32_t len)
{
    reset_all();
    uint32_t bankIdx = 0;
    MapleStateBank* dataBank = &g_dataBanks[bankIdx];
    dataBank->setOutPins(SDCKA_PIN, SDCKB_PIN);

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
            dataBank->setOutPins(SDCKA_PIN, SDCKB_PIN);
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
}

int main()
{
    gpio_init_mask(SDCKA_MASK | SDCKB_MASK);
    gpio_set_dir_out_masked(SDCKA_MASK | SDCKB_MASK);
    gpio_put_all(SDCKA_MASK | SDCKB_MASK);
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    systick_hw->rvr = CPU_TICKS_PER_PERIOD - 1;

    // This is just to give me a reference point where code starts to actually execute
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
