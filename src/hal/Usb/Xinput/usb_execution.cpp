#include "hal/Usb/DreamcastControllerObserver.hpp"

#include "configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usb_descriptors.h"
#include "class/hid/hid_device.h"
#include "xinput_device.hpp"

static void sendReportData(void)
{

  // Poll every 1ms
    const uint32_t interval_ms = 1;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return;  // not enough time
    start_ms += interval_ms;

    // Remote wakeup
    if (tud_suspended()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
    if (tud_xinput_n_ready(0))
    {
      tud_xinput_n_gamepad_report(0, 1, 0, 0, 0, 0, 0, 0, 0);
    }
}

DreamcastControllerObserver** get_usb_controller_observers()
{
    return NULL;
}

void usb_init()
{
  board_init();
  tusb_init();
}

void usb_task()
{
  sendReportData();
  tud_task(); // tinyusb device task
}

uint32_t get_num_usb_controllers()
{
    return 1;
}
