#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

// Going in reverse order because the host seems to usually enumerate the highest value first
enum {
    ITF_NUM_HID1 = 3,
    ITF_NUM_HID2 = 2,
    ITF_NUM_HID3 = 1,
    ITF_NUM_HID4 = 0
};

#define NUMBER_OF_DEVICES 4

#endif // __USB_DESCRITORS_H__
