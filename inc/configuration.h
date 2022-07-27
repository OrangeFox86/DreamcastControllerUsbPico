#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here
#define CPU_FREQ_KHZ 133000

// The minimum amount of time we check for an open line before taking control of it
#define MAPLE_OPEN_LINE_CHECK_TIME_US 10

// Amount of time in nanoseconds at which each bit transmits (value should be divisible by 3)
// 480 ns achieves just over 2 mbps, just as the Dreamcast does
#define MAPLE_NS_PER_BIT 480

// Added percentage on top of the expected write completion time
#define MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT 20

// Maximum amount of time waiting for the beginning of a response when one is expected
#define MAPLE_RESPONSE_TIMEOUT_US 500

// Default maximum amount of time to spend trying to read on the maple bus
// 4000 us accommodates the maximum number of words (256) at 2 mbps
#define DEFAULT_MAPLE_READ_TIMEOUT_US 4000

// The start pin of the two-pin bus for each player
#define P1_BUS_START_PIN 10
#define P2_BUS_START_PIN 12
#define P3_BUS_START_PIN 18
#define P4_BUS_START_PIN 20

// LED pin number for USB activity or -1 to disable
// When USB connected:
//   Default: ON
//   When controller key pressed: OFF
//   When controller disconnecting: Flashing slow
// When USB disconnected:
//   Default: OFF
//   When controller key pressed: Flashing quick
#define USB_LED_PIN 25

// LED pin number for simple USB activity or -1 to disable
// ON when USB connected; OFF when disconnected
#define SIMPLE_USB_LED_PIN -1

#endif // __CONFIGURATION_H__
