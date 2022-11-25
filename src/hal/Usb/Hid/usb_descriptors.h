#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

// Going in reverse order because the host seems to usually enumerate the highest value first
enum {
    // Gamepads
    ITF_NUM_GAMEPAD4 = 0,
    ITF_NUM_GAMEPAD3,
    ITF_NUM_GAMEPAD2,
    ITF_NUM_GAMEPAD1,
    // For mass storage device
    ITF_NUM_MSC,

    ITF_COUNT
};

#define NUMBER_OF_GAMEPADS (ITF_NUM_GAMEPAD1 + 1)

#endif // __USB_DESCRITORS_H__
