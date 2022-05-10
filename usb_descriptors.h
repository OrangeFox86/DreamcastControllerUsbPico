#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

#include "configuration.h"
#include "common/tusb_common.h"

#ifdef __cplusplus
 extern "C" {
#endif

// No one seems to follow the Tiny USB standard, so I'm defining my own
#define TUD_HID_REPORT_DESC_DREAMCAST_GAMEPAD() \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min 0, max 255 ) */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN  ( 0x00                                   ) ,\
    HID_LOGICAL_MAX  ( 0xFF                                   ) ,\
    HID_REPORT_COUNT ( 6                                      ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 4 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN  ( 1                                      ) ,\
    HID_LOGICAL_MAX  ( 8                                      ) ,\
    HID_PHYSICAL_MIN ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                               ) ,\
    HID_REPORT_COUNT ( 1                                      ) ,\
    HID_REPORT_SIZE  ( 4                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 12 bit Button Map */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN    ( 1                                      ) ,\
    HID_USAGE_MAX    ( 12                                     ) ,\
    HID_LOGICAL_MIN  ( 0                                      ) ,\
    HID_LOGICAL_MAX  ( 1                                      ) ,\
    HID_REPORT_COUNT ( 12                                     ) ,\
    HID_REPORT_SIZE  ( 1                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END

/// Dreamcast HID Gamepad Protocol Report.
typedef struct
{
  uint8_t  x;         ///< Delta x  movement of left analog-stick
  uint8_t  y;         ///< Delta y  movement of left analog-stick
  uint8_t  z;         ///< Delta z  movement of right analog-joystick
  uint8_t  rz;        ///< Delta Rz movement of right analog-joystick
  uint8_t  rx;        ///< Delta Rx movement of analog left trigger
  uint8_t  ry;        ///< Delta Ry movement of analog right trigger
  unsigned hat:4;     ///< 4 bit HAT (d-pad) value
  unsigned buttons:12; ///< Buttons mask for currently pressed buttons
}dreamcast_hid_gamepad_report_t;

#ifdef __cplusplus
 }
#endif

enum {
    ITF_NUM_HID1 = 0,
    ITF_NUM_TOTAL
};

#endif // __USB_DESCRITORS_H__
