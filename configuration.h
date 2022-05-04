#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here
#define CPU_FREQ_KHZ 133000

// Adjust the minimum time between each edge here
// 480 ns achieves just over 2 mbps, just as the dreamcast does
#define MAPLE_NS_PER_BIT 480

// The minimum amount of time we check for an open line before taking control of it
// This should be at least as long as the longest clock period of any device on the line.
// Note: The Dreamcast controller has a period of about 500 ns.
#define MAPLE_OPEN_LINE_CHECK_TIME_US 2

// Added percentage on top of the expected completion time
#define MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT 20

// Maximum amount of time waiting for response when one is expected
#define MAPLE_RESPONSE_TIMEOUT_US 500

// Default maximum amount of time to spend trying to read on the maple bus
// 4000 us accommodates the maximum number of words (256) at 2 mbps
#define DEFAULT_MAPLE_READ_TIMEOUT_US 4000

#endif // __CONFIGURATION_H__
