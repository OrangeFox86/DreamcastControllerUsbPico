#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

#include "configuration.h"
#include <stdint.h>

// Going in reverse order because the host seems to usually enumerate the highest value first
#define ITF_NUM_GAMEPAD(numGamepads, idx) (numGamepads - idx - 1)

#define NUMBER_OF_GAMEPADS (4)

// For mass storage device
#define ITF_NUM_MSC(numGamepads) (numGamepads)

#define ITF_NUM_CDC(numGamepads) (numGamepads + 1)
#define ITF_NUM_CDC_DATA(numGamepads) (numGamepads + 2)
#define ITF_COUNT(numGamepads) (numGamepads + 3)

//! Minumum analog value defined in USB HID descriptors
static const int8_t MIN_ANALOG_VALUE = -127;
//! Maximum analog value defined in USB HID descriptors
static const int8_t MAX_ANALOG_VALUE = 127;
//! Minimum trigger value defined in USB HID descriptors
static const int8_t MIN_TRIGGER_VALUE = 0;
//! Maximum trigger value defined in USB HID descriptors
static const int8_t MAX_TRIGGER_VALUE = MAX_ANALOG_VALUE;

#endif // __USB_DESCRITORS_H__
