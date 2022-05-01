#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here (note: overclocking doesn't help - keep at 133 MHz or less)
#define CPU_FREQ_MHZ 133
#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

// Adjust the minimum time between each edge here
// 320 ns achieves 2 mbps, just as the dreamcast does
#define MIN_CLOCK_PERIOD_NS 320

// The minimum amount of time we check for an open line before taking control of it
// This should be at least as long as the longest clock period of any device on the line.
// Note: The Dreamcast controller has a period of about 500 ns.
#define OPEN_LINE_CHECK_TIME_US 2

#endif // __CONFIGURATION_H__
