#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here (note: overclocking doesn't help - keep at 133 MHz or less)
#define CPU_FREQ_MHZ 133

// Adjust the minimum time between each edge here (in tests, timing has a variance of -0/+50 ns)
// - 300 ns should achieve a throughput between 1.5 and 2 mbps
// - The Dreamcast achieves an average throughput of about 2 mbps by clocking more efficiently
#define MIN_CLOCK_PERIOD_NS 300

// The minimum amount of time we check for an open line before taking control of it
// This should be at least as long as the longest clock period of any device on the line.
// Note: The Dreamcast controller has a period of about 500 ns.
#define OPEN_LINE_CHECK_TIME_NS 1500

// Maximum amount of time to wait for something else to start writing to the bus
#define DEFAULT_SYSTICK_READ_WAIT_US 500

// How long we should buffer input data before processing it
#define DEFAULT_SYSTICK_READ_TIMEOUT_US 1500

// Maximum number of input states (uint32) we can buffer within SYSTICK_READ_TIMEOUT_US
// 8 kiB should be enough to capture at least 80 words of data and take up ~13% of RAM
#define READ_BUFFER_SIZE (1024 * 8)

#endif // __CONFIGURATION_H__
