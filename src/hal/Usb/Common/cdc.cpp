#include "cdc.hpp"

#include "UsbControllerDevice.h"
#include "UsbGamepadDreamcastControllerObserver.hpp"
#include "UsbGamepad.h"
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
#include "class/cdc/cdc_device.h"

#if CFG_TUD_CDC

static MutexInterface* stdioMutex = nullptr;
static MutexInterface* rxMutex;
static std::vector<char>* rx;

// Can't use stdio_usb_init() because it checks tud_cdc_connected(), and that doesn't always return
// true when a connection is made. Not all terminal client set this when making connection.

#include "pico/stdio.h"
#include "pico/stdio/driver.h"
extern "C" {

static void stdio_usb_out_chars2(const char *buf, int length) {
    static uint64_t last_avail_time;

    LockGuard lockGuard(*stdioMutex);
    if (!lockGuard.isLocked())
    {
        return; // would deadlock otherwise
    }

    for (int i = 0; i < length;)
    {
        int n = length - i;
        int avail = (int) tud_cdc_write_available();
        if (n > avail) n = avail;
        if (n)
        {
            int n2 = (int) tud_cdc_write(buf + i, (uint32_t)n);
            tud_task();
            tud_cdc_write_flush();
            i += n2;
            last_avail_time = time_us_64();
        } else
        {
            tud_task();
            tud_cdc_write_flush();
            if (!tud_cdc_connected() ||
                (!tud_cdc_write_available() && time_us_64() > last_avail_time + 500000))
            {
                break;
            }
        }
    }
}

int stdio_usb_in_chars2(char *buf, int length)
{
    LockGuard lockGuard(*stdioMutex);
    if (!lockGuard.isLocked())
    {
        return PICO_ERROR_NO_DATA; // would deadlock otherwise
    }

    int rc = PICO_ERROR_NO_DATA;
    if (tud_cdc_connected() && tud_cdc_available())
    {
        int count = (int) tud_cdc_read(buf, (uint32_t) length);
        rc =  count ? count : PICO_ERROR_NO_DATA;
    }
    return rc;
}


struct stdio_driver stdio_usb2 =
{
    .out_chars = stdio_usb_out_chars2,
    .in_chars = stdio_usb_in_chars2,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = 0
#endif
};

} // extern "C"

void cdc_init(
    MutexInterface* cdcStdioMutex,
    MutexInterface* cdcRxMutex,
    std::vector<char>* cdcRx)
{
    stdioMutex = cdcStdioMutex;
    rxMutex = cdcRxMutex;
    rx = cdcRx;
    stdio_set_driver_enabled(&stdio_usb2, true);
}

void cdc_task()
{
#if USB_CDC_ENABLED
    // connected and there are data available
    if ( tud_cdc_available() )
    {
        char buf[64];
        uint32_t count = 0;

        {
            LockGuard lockGuard(*stdioMutex);
            if (!lockGuard.isLocked())
            {
                return; // would deadlock otherwise; just wait for next loop
            }

            // read data
            count = tud_cdc_read(buf, sizeof(buf));

            if (count > 0)
            {
                // Echo back
                tud_cdc_write(buf, count);
                tud_cdc_write_flush();
            }
        }

        if (count > 0)
        {
            LockGuard lockGuard(*stdioMutex, true);
            rx->reserve(rx->size() + count);
            for (uint32_t i = 0; i < count; ++i)
            {
                rx->push_back(buf[i]);
            }
        }
    }
#endif
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}

#endif // #if CFG_TUD_CDC
