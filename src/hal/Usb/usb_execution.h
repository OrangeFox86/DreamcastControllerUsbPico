#ifndef __USB_EXECUTION_H__
#define __USB_EXECUTION_H__

#include "hal/Usb/UsbControllerInterface.hpp"
#include <stdint.h>

//! Sets all of the USB devices to execute with
void set_usb_devices(UsbControllerInterface** devices, uint8_t n);
//! USB initialization
void usb_init();
//! USB task that needs to be called constantly by main()
void usb_task();

#endif // __USB_EXECUTION_H__
