#ifndef __USB_DESCRIPTORS_H__
#define __USB_DESCRIPTORS_H__

// Going in reverse order because the host seems to usually enumerate the highest value first
enum {
    ITF_NUM_XINPUT1 = 0
};

#define NUMBER_OF_GAMEPADS 1

#define TUD_XINPUT_DESC_LEN (9 + 17 + 7 + 7)
#define TUD_XINPUT_NUM_ENDPOINTS 2

#define TUD_XINPUT_DESCRIPTOR(_itfnum, _stridx, _epin, _epout) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, TUD_XINPUT_NUM_ENDPOINTS, TUSB_CLASS_VENDOR_SPECIFIC, 0x5D, 0x01, _stridx,\
  /* Unknown */ \
  17, TUSB_DESC_FUNCTIONAL, 0x00, 0x01, 0x01, 0x25, _epin, 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, _epout, 0x08, 0x00, 0x00,  \
  /* Button device output */ \
  7, TUSB_DESC_ENDPOINT, _epin, 0x03, 0x20, 0x00, 0x04, \
  /* Force feedback and LED device input */ \
  7, TUSB_DESC_ENDPOINT, _epout, 0x03, 0x20, 0x00, 0x08

#endif // __USB_DESCRIPTORS_H__
