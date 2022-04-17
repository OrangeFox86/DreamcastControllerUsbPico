// Adjust the CPU clock frequency here
#define CPU_FREQ_MHZ 133
#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

// Adjust the minimum time between each bit bang here
#define CLOCK_PERIOD_NS 320
#define CPU_TICKS_PER_PERIOD (int)(CLOCK_PERIOD_NS * CPU_FREQ_MHZ / 1000.0 + 0.5)

// The size of this doesn't represent bits but rather max number of state changes in a bank.
// Increasing this will increase RAM usage and may increase data encode time but decrease number
// of data stalls during write; decreasing will do the opposite of all that.
#define ELEMENTS_PER_BANK 256
