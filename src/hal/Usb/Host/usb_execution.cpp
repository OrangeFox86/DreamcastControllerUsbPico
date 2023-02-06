#include "hal/Usb/host_usb_interface.hpp"

#include "tusb_config.h"
#include "bsp/board.h"
#include "tusb.h"

void usb_init()
{
    board_init();
    tusb_init();
}

void usb_task()
{
    tuh_task();
}
