#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#include "DreamcastControllerObserver.hpp"

//! @returns array of the USB controller observers
DreamcastControllerObserver** get_usb_controller_observers();
//! USB initialization
void usb_init();
//! USB task that needs to be called constantly by main()
void usb_task();
//! @returns number of USB controllers
uint32_t get_num_usb_controllers();

#endif // __USB_INTERFACE_H__
