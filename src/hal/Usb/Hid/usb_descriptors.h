#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

#include "configuration.h"

// Going in reverse order because the host seems to usually enumerate the highest value first
enum {
    // Gamepads
    ITF_NUM_GAMEPAD4 = 0,
    ITF_NUM_GAMEPAD3,
    ITF_NUM_GAMEPAD2,
    ITF_NUM_GAMEPAD1,
    // For mass storage device
    ITF_NUM_MSC,

#if USB_CDC_ENABLED
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
#endif

    ITF_COUNT
};

#define NUMBER_OF_GAMEPADS (ITF_NUM_GAMEPAD1 + 1)

#define HID_OUTPUT_N(x, n)          HID_REPORT_ITEM(x, RI_MAIN_OUTPUT        , RI_TYPE_MAIN, n)

/// HID Usage Table - (PID) Table 2: Physical Input Device Page
enum {
  HID_USAGE_PID_PID                                 = 0x01,
  HID_USAGE_PID_NORMAL                              = 0x20,
  HID_USAGE_PID_SET_EFFECT_REPORT                   = 0x21,
  HID_USAGE_PID_EFFECT_BLOCK_INDEX                  = 0x22,
  HID_USAGE_PID_PARAMETER_BLOCK_OFFSET              = 0x23,
  HID_USAGE_PID_ROM_FLAG                            = 0x24,
  HID_USAGE_PID_EFFECT_TYPE                         = 0x25,
  HID_USAGE_PID_ET_CONSTANT_FORCE                   = 0x26,
  HID_USAGE_PID_ET_RAMP                             = 0x27,
  HID_USAGE_PID_ET_CUSTOM_FORCE_DATA                = 0x28,
  HID_USAGE_PID_ET_SQUARE                           = 0x30,
  HID_USAGE_PID_ET_SINE                             = 0x31,
  HID_USAGE_PID_ET_TRIANGLE                         = 0x32,
  HID_USAGE_PID_ET_SAWTOOTH_UP                      = 0x33,
  HID_USAGE_PID_ET_SAWTOOTH_DOWN                    = 0x34,
  HID_USAGE_PID_ET_SPRING                           = 0x40,
  HID_USAGE_PID_ET_DAMPER                           = 0x41,
  HID_USAGE_PID_ET_INERTIA                          = 0x42,
  HID_USAGE_PID_ET_FRICTION                         = 0x43,
  HID_USAGE_PID_DURATION                            = 0x50,
  HID_USAGE_PID_SAMPLE_PERIOD                       = 0x51,
  HID_USAGE_PID_GAIN                                = 0x52,
  HID_USAGE_PID_TRIGGER_BUTTON                      = 0x53,
  HID_USAGE_PID_TRIGGER_REPEAT_INTERVAL             = 0x54,
  HID_USAGE_PID_AXES_ENABLE                         = 0x55,
  HID_USAGE_PID_DIRECTION_ENABLE                    = 0x56,
  HID_USAGE_PID_DIRECTION                           = 0x57,
  HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET          = 0x58,
  HID_USAGE_PID_BLOCK_TYPE                          = 0x59,
  HID_USAGE_PID_SET_ENVELOPE_REPORT                 = 0x5A,
  HID_USAGE_PID_ATTACK_LEVEL                        = 0x5B,
  HID_USAGE_PID_ATTACK_TIME                         = 0x5C,
  HID_USAGE_PID_FADE_LEVEL                          = 0x5D,
  HID_USAGE_PID_FADE_TIME                           = 0x5E,
  HID_USAGE_PID_SET_CONDITION_REPORT                = 0x5F,
  HID_USAGE_PID_CP_OFFSET                           = 0x60,
  HID_USAGE_PID_POSITIVE_COEFFICIENT                = 0x61,
  HID_USAGE_PID_NEGATIVE_COEFFICIENT                = 0x62,
  HID_USAGE_PID_POSITIVE_SATURATION                 = 0x63,
  HID_USAGE_PID_NEGATIVE_SATURATION                 = 0x64,
  HID_USAGE_PID_DEAD_BAND                           = 0x65,
  HID_USAGE_PID_DOWNLOAD_FORCE_SAMPLE               = 0x66,
  HID_USAGE_PID_ISOCH_CUSTOM_FORCE_ENABLE           = 0x67,
  HID_USAGE_PID_CUSTOM_FORCE_DATA_REPORT            = 0x68,
  HID_USAGE_PID_CUSTOM_FORCE_DATA                   = 0x69,
  HID_USAGE_PID_CUSTOM_FORCE_VENDOR_DEFINED_DATA    = 0x6A,
  HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT             = 0x6B,
  HID_USAGE_PID_CUSTOM_FORCE_DATA_OFFSET            = 0x6C,
  HID_USAGE_PID_SAMPLE_COUNT                        = 0x6D,
  HID_USAGE_PID_SET_PERIODIC_REPORT                 = 0x6E,
  HID_USAGE_PID_OFFSET                              = 0x6F,
  HID_USAGE_PID_MAGNITUDE                           = 0x70,
  HID_USAGE_PID_PHASE                               = 0x71,
  HID_USAGE_PID_PERIOD                              = 0x72,
  HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT           = 0x73,
  HID_USAGE_PID_SET_RAMP_FORCE_REPORT               = 0x74,
  HID_USAGE_PID_RAMP_START                          = 0x75,
  HID_USAGE_PID_RAMP_END                            = 0x76,
  HID_USAGE_PID_EFFECT_OPERATION_REPORT             = 0x77,
  HID_USAGE_PID_EFFECT_OPERATION                    = 0x78,
  HID_USAGE_PID_OP_EFFECT_START                     = 0x79,
  HID_USAGE_PID_OP_EFFECT_START_SOLO                = 0x7A,
  HID_USAGE_PID_OP_EFFECT_STOP                      = 0x7B,
  HID_USAGE_PID_LOOP_COUNT                          = 0x7C,
  HID_USAGE_PID_DEVICE_GAIN_REPORT                  = 0x7D,
  HID_USAGE_PID_DEVICE_GAIN                         = 0x7E,
  HID_USAGE_PID_PID_POOL_REPORT                     = 0x7F,
  HID_USAGE_PID_RAM_POOL_SIZE                       = 0x80,
  HID_USAGE_PID_ROM_POOL_SIZE                       = 0x81,
  HID_USAGE_PID_ROM_EFFECT_BLOCK_COUNT              = 0x82,
  HID_USAGE_PID_SIMULTANEOUS_EFFECTS_MAX            = 0x83,
  HID_USAGE_PID_POOL_ALIGNMENT                      = 0x84,
  HID_USAGE_PID_PID_POOL_MOVE_REPORT                = 0x85,
  HID_USAGE_PID_MOVE_SOURCE                         = 0x86,
  HID_USAGE_PID_MOVE_DESTINATION                    = 0x87,
  HID_USAGE_PID_MOVE_LENGTH                         = 0x88,
  HID_USAGE_PID_PID_BLOCK_LOAD_REPORT               = 0x89,
  HID_USAGE_PID_BLOCK_LOAD_STATUS                   = 0x8B,
  HID_USAGE_PID_BLOCK_LOAD_SUCCESS                  = 0x8C,
  HID_USAGE_PID_BLOCK_LOAD_FULL                     = 0x8D,
  HID_USAGE_PID_BLOCK_LOAD_ERROR                    = 0x8E,
  HID_USAGE_PID_BLOCK_HANDLE                        = 0x8F,
  HID_USAGE_PID_PID_BLOCK_FREE_REPORT               = 0x90,
  HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_HANDLE          = 0x91,
  HID_USAGE_PID_PID_STATE_REPORT                    = 0x92,
  HID_USAGE_PID_EFFECT_PLAYING                      = 0x94,
  HID_USAGE_PID_PID_DEVICE_CONTROL_REPORT           = 0x95,
  HID_USAGE_PID_PID_DEVICE_CONTROL                  = 0x96,
  HID_USAGE_PID_DC_ENABLE_ACTUATORS                 = 0x97,
  HID_USAGE_PID_DC_DISABLE_ACTUATORS                = 0x98,
  HID_USAGE_PID_DC_STOP_ALL_EFFECTS                 = 0x99,
  HID_USAGE_PID_DC_DEVICE_RESET                     = 0x9A,
  HID_USAGE_PID_DC_DEVICE_PAUSE                     = 0x9B,
  HID_USAGE_PID_DC_DEVICE_CONTINUE                  = 0x9C,
  HID_USAGE_PID_DEVICE_PAUSED                       = 0x9F,
  HID_USAGE_PID_ACTUATORS_ENABLED                   = 0xA0,
  HID_USAGE_PID_SAFETY_SWITCH                       = 0xA4,
  HID_USAGE_PID_ACTUATOR_OVERRIDE_SWITCH            = 0xA5,
  HID_USAGE_PID_ACTUATOR_POWER                      = 0xA6,
  HID_USAGE_PID_START_DELAY                         = 0xA7,
  HID_USAGE_PID_PARAMETER_BLOCK_SIZE                = 0xA8,
  HID_USAGE_PID_DEVICE_MANAGED_POOL                 = 0xA9,
  HID_USAGE_PID_SHARED_PARAMETER_BLOCKS             = 0xAA,
  HID_USAGE_PID_CREATE_NEW_EFFECT_REPORT            = 0xAB,
  HID_USAGE_PID_RAM_POOL_AVAILABLE                  = 0xAC,
};

// Gamepad Report Descriptor (based on TUD_HID_REPORT_DESC_GAMEPAD)
// with 16 buttons, 2 joysticks and 1 hat/dpad with following layout
// | X | Y | Z | Rz | Rx | Ry (1 byte each) | hat/DPAD (1 byte) | Button Map (2 bytes) |
#define TUD_HID_REPORT_DESC_GAMEPAD_W_FORCE_FEEDBACK() \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_LOGICAL_MIN(0), \
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    HID_REPORT_ID(1) \
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min -127, max 127 ) */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN  ( 0x81                                   ) ,\
    HID_LOGICAL_MAX  ( 0x7f                                   ) ,\
    HID_REPORT_COUNT ( 6                                      ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE        ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN  ( 1                                      ) ,\
    HID_LOGICAL_MAX  ( 8                                      ) ,\
    HID_PHYSICAL_MIN ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                               ) ,\
    /* Eng rot:Angular pos */ \
    HID_UNIT_N        ( 0x0014, 2                              ) ,\
    HID_REPORT_COUNT ( 1                                      ) ,\
    HID_REPORT_SIZE  ( 8                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 16 bit Button Map */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN    ( 1                                      ) ,\
    HID_USAGE_MAX    ( 32                                     ) ,\
    HID_LOGICAL_MIN  ( 0                                      ) ,\
    HID_LOGICAL_MAX  ( 1                                      ) ,\
    HID_REPORT_COUNT ( 32                                     ) ,\
    HID_REPORT_SIZE  ( 1                                      ) ,\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* Force Feedback */ \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_PID                     ) ,\
    HID_USAGE        ( HID_USAGE_PID_SET_EFFECT_REPORT        ) ,\
    HID_COLLECTION   ( HID_COLLECTION_LOGICAL                 ) ,\
      /* 7 bits for effect block index */ \
      HID_USAGE         ( HID_USAGE_PID_EFFECT_BLOCK_INDEX       ) ,\
      HID_LOGICAL_MAX   ( 127                                    ) ,\
      HID_REPORT_SIZE   ( 7                                      ) ,\
      HID_REPORT_COUNT  ( 1                                      ) ,\
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      /* 1 bit for ROM flag */ \
      HID_USAGE         ( HID_USAGE_PID_ROM_FLAG                 ) ,\
      HID_LOGICAL_MAX   ( 1                                      ) ,\
      HID_REPORT_SIZE   ( 1                                      ) ,\
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      /* Available effect types */ \
      HID_USAGE         ( HID_USAGE_PID_EFFECT_TYPE              ) ,\
      HID_COLLECTION    ( HID_COLLECTION_LOGICAL                 ) ,\
        HID_USAGE         ( HID_USAGE_PID_ET_CONSTANT_FORCE        ) ,\
        HID_USAGE         ( HID_USAGE_PID_ET_RAMP                  ) ,\
        HID_USAGE(HID_USAGE_PID_ET_SQUARE), \
        HID_USAGE         ( HID_USAGE_PID_ET_SINE                  ) ,\
        HID_USAGE(HID_USAGE_PID_ET_TRIANGLE), \
        HID_USAGE(HID_USAGE_PID_ET_SAWTOOTH_UP), \
        HID_USAGE(HID_USAGE_PID_ET_SAWTOOTH_DOWN), \
        HID_USAGE(HID_USAGE_PID_ET_SPRING), \
        HID_USAGE(HID_USAGE_PID_ET_DAMPER), \
        HID_USAGE(HID_USAGE_PID_ET_INERTIA), \
        HID_LOGICAL_MIN   ( 1                                      ) ,\
        HID_LOGICAL_MAX   ( 10                                      ) ,\
        HID_REPORT_SIZE   ( 8                                      ) ,\
        HID_OUTPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,\
      HID_COLLECTION_END                                             ,\
      HID_USAGE         ( HID_USAGE_PID_DURATION                 ) ,\
      HID_USAGE         ( HID_USAGE_PID_TRIGGER_REPEAT_INTERVAL  ) ,\
      HID_LOGICAL_MIN   ( 0                                      ) ,\
      HID_LOGICAL_MAX_N ( 10000, 2                               ) ,\
      HID_PHYSICAL_MAX_N( 10000, 2                               ) ,\
      HID_REPORT_SIZE   ( 16                                     ) ,\
      /* Eng Lin:Time */ \
      HID_UNIT_N        ( 0x1003, 2                              ) ,\
      /* -3 */ \
      HID_UNIT_EXPONENT ( 16 - 3                                 ) ,\
      HID_REPORT_COUNT  ( 2                                      ) ,\
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      /* -6 */ \
      HID_UNIT_EXPONENT ( 16 - 6                                 ) ,\
      HID_USAGE         ( HID_USAGE_PID_SAMPLE_PERIOD            ) ,\
      HID_REPORT_COUNT  ( 1                                      ) ,\
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      HID_PHYSICAL_MAX  ( 0                                      ) ,\
      HID_UNIT_EXPONENT ( 0                                      ) ,\
      HID_UNIT          ( 0                                      ) ,\
      HID_USAGE         ( HID_USAGE_PID_GAIN                     ) ,\
      HID_USAGE         ( HID_USAGE_PID_TRIGGER_BUTTON           ) ,\
      HID_LOGICAL_MAX   ( 127                                    ) ,\
      HID_REPORT_SIZE   ( 8                                      ) ,\
      HID_REPORT_COUNT  ( 2                                      ) ,\
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      /* Tie these axes to the stick */ \
      HID_USAGE         ( HID_USAGE_PID_AXES_ENABLE              ) ,\
      HID_COLLECTION    ( HID_COLLECTION_LOGICAL                 ) ,\
        HID_USAGE_PAGE    ( HID_USAGE_PAGE_DESKTOP                 ) ,\
        HID_USAGE         ( HID_USAGE_DESKTOP_POINTER              ) ,\
        HID_COLLECTION    ( HID_COLLECTION_PHYSICAL                ) ,\
          HID_USAGE         ( HID_USAGE_DESKTOP_X                    ) ,\
          HID_USAGE         ( HID_USAGE_DESKTOP_Y                    ) ,\
          HID_LOGICAL_MAX   ( 1                                      ) ,\
          HID_REPORT_SIZE   ( 1                                      ) ,\
          HID_REPORT_COUNT  ( 2                                      ) ,\
          HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
        HID_COLLECTION_END                                           ,\
      HID_COLLECTION_END                                           ,\
      /* 6-bit pad */ \
      HID_REPORT_COUNT  ( 6                                      ) ,\
      HID_OUTPUT        ( HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE ) ,\
      HID_USAGE_PAGE    ( HID_USAGE_PAGE_PID                     ) ,\
      HID_USAGE         ( HID_USAGE_PID_DIRECTION                ) ,\
      HID_COLLECTION    ( HID_COLLECTION_LOGICAL                 ) ,\
        HID_USAGE_PAGE    ( HID_USAGE_PAGE_DESKTOP                 ) ,\
        /* Tie these axes back to the stick */ \
        HID_USAGE         ( HID_USAGE_DESKTOP_POINTER              ) ,\
        HID_COLLECTION    ( HID_COLLECTION_PHYSICAL                ) ,\
          HID_USAGE         ( HID_USAGE_DESKTOP_X                    ) ,\
          HID_USAGE         ( HID_USAGE_DESKTOP_Y                    ) ,\
          HID_LOGICAL_MIN   ( 0                                      ) ,\
          HID_LOGICAL_MAX_N ( 255, 2                                 ) ,\
          HID_PHYSICAL_MAX_N( 360, 2                                 ) ,\
          /* Eng rot:Angular pos */ \
          HID_UNIT_N        ( 0x0014, 2                              ) ,\
          HID_REPORT_SIZE   ( 8                                      ) ,\
          HID_REPORT_COUNT  ( 2                                      ) ,\
          HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
          HID_UNIT          ( 0                                      ) ,\
          HID_PHYSICAL_MAX  ( 0                                      ) ,\
        HID_COLLECTION_END                                           ,\
      HID_COLLECTION_END                                           ,\
      HID_USAGE_PAGE    ( HID_USAGE_PAGE_PID                     ) ,\
      HID_USAGE         ( HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET ) ,\
      HID_COLLECTION    ( HID_COLLECTION_LOGICAL                 ) ,\
        /* Ordinals:Instance 1 */ \
        HID_USAGE_N       ( 0x000A0001, 3                          ) ,\
        /* Ordinals:Instance 2 */ \
        HID_USAGE_N       ( 0x000A0002, 3                          ) ,\
        /* 32K RAM or ROM max. */ \
        HID_LOGICAL_MAX_N ( 32765, 2                               ) ,\
        HID_REPORT_SIZE   ( 16                                     ) ,\
        HID_REPORT_COUNT  ( 2                                      ) ,\
        HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      HID_COLLECTION_END                                           ,\
    HID_COLLECTION_END                                           ,\
    /* Envelope Report Definition*/ \
    HID_USAGE(HID_USAGE_PID_SET_ENVELOPE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(2) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      /* 32K RAM or ROM max. */ \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG),\
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ATTACK_LEVEL), \
      HID_USAGE(HID_USAGE_PID_FADE_LEVEL), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT(2), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ATTACK_TIME), \
      HID_USAGE(HID_USAGE_PID_FADE_TIME), \
      HID_LOGICAL_MAX_N(10000, 2), \
      HID_PHYSICAL_MAX_N(10000, 2), \
      /* Eng Lin:Time */ \
      HID_UNIT_N        ( 0x1003, 2                              ) ,\
      /* -3 */ \
      HID_UNIT_EXPONENT ( 16 - 3                                 ) ,\
      HID_REPORT_SIZE(16), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_PHYSICAL_MAX(0), \
      HID_UNIT(0), \
      HID_UNIT_EXPONENT(0), \
    HID_COLLECTION_END, \
    /* Condition Report Definition */ \
    HID_USAGE(HID_USAGE_PID_SET_CONDITION_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(3) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG), \
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_CP_OFFSET), \
      HID_USAGE(HID_USAGE_PID_POSITIVE_COEFFICIENT), \
      HID_USAGE(HID_USAGE_PID_NEGATIVE_COEFFICIENT), \
      HID_USAGE(HID_USAGE_PID_POSITIVE_SATURATION), \
      HID_USAGE(HID_USAGE_PID_NEGATIVE_SATURATION), \
      HID_USAGE(HID_USAGE_PID_DEAD_BAND), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT(6), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* Periodic Report Definition */\
    HID_USAGE(HID_USAGE_PID_SET_PERIODIC_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(4) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG), \
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_MAGNITUDE), \
      HID_USAGE(HID_USAGE_PID_OFFSET), \
      HID_USAGE(HID_USAGE_PID_PHASE), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT(3), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_PERIOD), \
      HID_LOGICAL_MAX_N(10000, 2), \
      HID_PHYSICAL_MAX_N(10000, 2), \
      /* Eng Lin:Time */ \
      HID_UNIT_N        ( 0x1003, 2                              ) ,\
      /* -3 */ \
      HID_UNIT_EXPONENT ( 16 - 3                                 ) ,\
      HID_REPORT_SIZE(16), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_PHYSICAL_MAX(0), \
      HID_UNIT(0), \
      HID_UNIT_EXPONENT(0), \
    HID_COLLECTION_END, \
    /* Constant Force Report Definition */ \
    HID_USAGE(HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(5) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG),\
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_MAGNITUDE), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* Ramp Force Report Definition */ \
    HID_USAGE(HID_USAGE_PID_SET_RAMP_FORCE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(6) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG),\
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_RAMP_START), \
      HID_USAGE(HID_USAGE_PID_RAMP_END), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT(2), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* Custom Force Data Report Definition */ \
    HID_USAGE(HID_USAGE_PID_CUSTOM_FORCE_DATA_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(7) \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_LOGICAL_MAX_N(32765, 2), \
      HID_REPORT_SIZE(15), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_USAGE_N(HID_USAGE_DESKTOP_BYTE_COUNT | (HID_USAGE_PAGE_DESKTOP << 16), 3), \
      HID_LOGICAL_MAX_N(256, 2), \
      HID_REPORT_SIZE(9), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_CUSTOM_FORCE_DATA), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT_N(256, 2), \
      HID_OUTPUT_N        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_BUFFERED_BYTES, 2), \
    HID_COLLECTION_END, \
    /* Download Force Sample Definition */ \
    HID_USAGE(HID_USAGE_PID_DOWNLOAD_FORCE_SAMPLE), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(8) \
      HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), \
      HID_USAGE(HID_USAGE_DESKTOP_POINTER), \
      HID_COLLECTION(HID_COLLECTION_LOGICAL), \
        HID_USAGE(HID_USAGE_DESKTOP_X), \
        HID_USAGE(HID_USAGE_DESKTOP_Y), \
        HID_LOGICAL_MIN(-127), \
        HID_LOGICAL_MAX(127), \
        HID_REPORT_SIZE(8), \
        HID_REPORT_COUNT(2), \
        HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_COLLECTION_END, \
    HID_COLLECTION_END, \
    HID_USAGE_PAGE   ( HID_USAGE_PAGE_PID                     ) ,\
    /* Define the Custom Force Parameter Block */ \
    HID_USAGE(HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(9) \
      /* Parameter block offset in pool */ \
      /* Custom Force data offset in pool */ \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_OFFSET), \
      HID_USAGE(HID_USAGE_PID_CUSTOM_FORCE_DATA_OFFSET), \
      HID_USAGE(HID_USAGE_PID_SAMPLE_COUNT), \
      HID_LOGICAL_MIN(0), \
      /* 32K RAM or ROM max. */ \
      HID_LOGICAL_MAX_N ( 32765, 2                               ) ,\
      HID_REPORT_COUNT(3), \
      HID_REPORT_SIZE(16), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* Effect Operation Report Definition */ \
    HID_USAGE(HID_USAGE_PID_EFFECT_OPERATION_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(10) \
      HID_USAGE(HID_USAGE_PID_EFFECT_BLOCK_INDEX), \
      HID_LOGICAL_MAX(127), \
      HID_REPORT_SIZE(7), \
      HID_REPORT_COUNT(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG),\
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_EFFECT_OPERATION), \
      HID_COLLECTION(HID_COLLECTION_LOGICAL), \
        HID_USAGE(HID_USAGE_PID_OP_EFFECT_START), \
        HID_USAGE(HID_USAGE_PID_OP_EFFECT_START_SOLO), \
        HID_USAGE(HID_USAGE_PID_OP_EFFECT_STOP), \
        HID_LOGICAL_MIN(1), \
        HID_LOGICAL_MAX(3), \
        HID_REPORT_SIZE(8), \
        HID_OUTPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ), \
      HID_COLLECTION_END, \
      \
      HID_USAGE(HID_USAGE_PID_LOOP_COUNT), \
      HID_LOGICAL_MIN(0), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_OUTPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* PID Pool Report Definition */ \
    HID_USAGE(HID_USAGE_PID_PID_POOL_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(1) \
      HID_USAGE(HID_USAGE_PID_RAM_POOL_SIZE), \
      HID_USAGE(HID_USAGE_PID_ROM_POOL_SIZE), \
      HID_USAGE(HID_USAGE_PID_ROM_EFFECT_BLOCK_COUNT), \
      HID_LOGICAL_MAX_N ( 32765, 2                               ) ,\
      HID_REPORT_COUNT(3), \
      HID_REPORT_SIZE(16), \
      HID_FEATURE( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      \
      HID_USAGE(HID_USAGE_PID_PARAMETER_BLOCK_SIZE), \
      HID_COLLECTION(HID_COLLECTION_LOGICAL), \
        HID_USAGE(HID_USAGE_PID_SET_EFFECT_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_ENVELOPE_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_CONDITION_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_PERIODIC_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_RAMP_FORCE_REPORT), \
        HID_USAGE(HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT), \
        HID_LOGICAL_MAX_N(255, 2), \
        HID_REPORT_SIZE(8), \
        HID_REPORT_COUNT(7), \
        HID_FEATURE( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_COLLECTION_END, \
      \
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(7), \
      HID_REPORT_COUNT(1), \
      /* 7-bit pad */ \
      HID_FEATURE( HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE ), \
      HID_USAGE(HID_USAGE_PID_ISOCH_CUSTOM_FORCE_ENABLE), \
      HID_REPORT_SIZE(1), \
      HID_FEATURE( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
    /* PID State Report Definition */ \
    HID_USAGE(HID_USAGE_PID_PID_STATE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(2) \
      HID_USAGE(HID_USAGE_PID_EFFECT_BLOCK_INDEX), \
      HID_LOGICAL_MAX(127), \
      HID_REPORT_SIZE(7), \
      HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      \
      HID_USAGE(HID_USAGE_PID_ROM_FLAG), \
      HID_LOGICAL_MAX(1), \
      HID_REPORT_SIZE(1), \
      HID_REPORT_COUNT(1), \
      HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      \
      HID_USAGE(HID_USAGE_PID_EFFECT_PLAYING), \
      HID_USAGE(HID_USAGE_PID_ACTUATORS_ENABLED), \
      HID_USAGE(HID_USAGE_PID_SAFETY_SWITCH), \
      HID_USAGE(HID_USAGE_PID_ACTUATOR_POWER), \
      HID_REPORT_SIZE(1), \
      HID_REPORT_COUNT(4), \
      HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      \
      /* 4-bit Pad */ \
      HID_INPUT        ( HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE ) ,\
    HID_COLLECTION_END, \
    /* PID Device Control Report Definition */ \
    HID_USAGE(HID_USAGE_PID_PID_DEVICE_CONTROL_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(11) \
      HID_USAGE(HID_USAGE_PID_PID_DEVICE_CONTROL), \
      HID_COLLECTION(HID_COLLECTION_LOGICAL), \
        HID_USAGE(HID_USAGE_PID_DC_ENABLE_ACTUATORS), \
        HID_USAGE(HID_USAGE_PID_DC_DISABLE_ACTUATORS), \
        HID_USAGE(HID_USAGE_PID_DC_STOP_ALL_EFFECTS), \
        HID_USAGE(HID_USAGE_PID_DC_DEVICE_RESET), \
        HID_USAGE(HID_USAGE_PID_DC_DEVICE_PAUSE), \
        HID_USAGE(HID_USAGE_PID_DC_DEVICE_CONTINUE), \
        HID_LOGICAL_MIN(1), \
        HID_LOGICAL_MAX(6), \
        HID_REPORT_SIZE(1), \
        /* PID documentation labels 4 but specifies 08 */ \
        HID_REPORT_COUNT(8), \
        /* CHECK: data,ary,abs */ \
        0x91, 0x02, \
      HID_COLLECTION_END, \
    HID_COLLECTION_END, \
    /* PID Pool Move Report Definition */ \
    HID_USAGE(HID_USAGE_PID_PID_POOL_MOVE_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(12) \
      HID_USAGE(HID_USAGE_PID_MOVE_SOURCE), \
      HID_USAGE(HID_USAGE_PID_MOVE_DESTINATION), \
      HID_USAGE(HID_USAGE_PID_MOVE_LENGTH), \
      HID_LOGICAL_MAX_N(32767, 2), \
      HID_REPORT_SIZE(16), \
      HID_REPORT_COUNT(3), \
      HID_OUTPUT_N        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_BUFFERED_BYTES, 2), \
    HID_COLLECTION_END, \
    /* Device Gain Report Definition */ \
    HID_USAGE(HID_USAGE_PID_DEVICE_GAIN_REPORT), \
    HID_COLLECTION(HID_COLLECTION_LOGICAL), \
      HID_REPORT_ID(2) \
      HID_USAGE(HID_USAGE_PID_DEVICE_GAIN), \
      HID_LOGICAL_MAX_N(255, 2), \
      HID_REPORT_SIZE(8), \
      HID_REPORT_COUNT(1), \
      HID_FEATURE( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    HID_COLLECTION_END, \
  HID_COLLECTION_END


#endif // __USB_DESCRITORS_H__
