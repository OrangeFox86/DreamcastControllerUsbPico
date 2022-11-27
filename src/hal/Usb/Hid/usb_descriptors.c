/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid_device.h"
#include "pico/unique_id.h"
#include "configuration.h"
#include <string.h>

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    // VID 1209 comes from https://pid.codes/
    // PID 2F07 is a subassignment granted through github
    // https://github.com/pidcodes/pidcodes.github.com/blob/74f95539d2ad737c1ba2871eeb25b3f5f5d5c790/1209/2F07/index.md
    .idVendor           = 0x1209,
    .idProduct          = 0x2F07,

    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report1[] =
{
    TUD_HID_REPORT_DESC_GAMEPAD()
};

uint8_t const desc_hid_report2[] =
{
    TUD_HID_REPORT_DESC_GAMEPAD()
};

uint8_t const desc_hid_report3[] =
{
    TUD_HID_REPORT_DESC_GAMEPAD()
};

uint8_t const desc_hid_report4[] =
{
    TUD_HID_REPORT_DESC_GAMEPAD()
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    switch (instance)
    {
        case ITF_NUM_GAMEPAD1:
            return desc_hid_report1;
        case ITF_NUM_GAMEPAD2:
            return desc_hid_report2;
        case ITF_NUM_GAMEPAD3:
            return desc_hid_report3;
        case ITF_NUM_GAMEPAD4:
            return desc_hid_report4;
        default:
            return NULL;
    }
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#if CDC_ENABLED
    #define DEBUG_CONFIG_LEN TUD_CDC_DESC_LEN
#else
    #define DEBUG_CONFIG_LEN 0
#endif

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + (NUMBER_OF_GAMEPADS * TUD_HID_DESC_LEN) + DEBUG_CONFIG_LEN + TUD_MSC_DESC_LEN)

// Endpoint definitions (must all be unique)
#define EPIN_GAMEPAD1   (0x84)
#define EPIN_GAMEPAD2   (0x83)
#define EPIN_GAMEPAD3   (0x82)
#define EPIN_GAMEPAD4   (0x81)
#define EPOUT_MSC       (0x05)
#define EPIN_MSC        (0x85)
#define EPIN_CDC_NOTIF  (0x86)
#define EPOUT_CDC       (0x07)
#define EPIN_CDC        (0x87)

#define GAMEPAD_REPORT_SIZE (1 + sizeof(hid_gamepad_report_t))

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 400),

    // *************************************************************************
    // * Gamepad Descriptors                                                   *
    // *************************************************************************

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval ms
    TUD_HID_DESCRIPTOR(ITF_NUM_GAMEPAD4, 7, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report4),
                                EPIN_GAMEPAD4, GAMEPAD_REPORT_SIZE, 1),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval ms
    TUD_HID_DESCRIPTOR(ITF_NUM_GAMEPAD3, 6, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report3),
                                EPIN_GAMEPAD3, GAMEPAD_REPORT_SIZE, 1),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval ms
    TUD_HID_DESCRIPTOR(ITF_NUM_GAMEPAD2, 5, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report2),
                                EPIN_GAMEPAD2, GAMEPAD_REPORT_SIZE, 1),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval ms
    TUD_HID_DESCRIPTOR(ITF_NUM_GAMEPAD1, 4, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1),
                                EPIN_GAMEPAD1, GAMEPAD_REPORT_SIZE, 1),

    // *************************************************************************
    // * Storage Device Descriptor                                             *
    // *************************************************************************

    // Only doing transfer at full speed since each file will only be about 128KB, max of 8 files

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 8, EPOUT_MSC, EPIN_MSC, 64),

    // *************************************************************************
    // * Communication Device Descriptor  (for debug messaging)                *
    // *************************************************************************

#if CDC_ENABLED
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 9, EPIN_CDC_NOTIF, 8, EPOUT_CDC, EPIN_CDC, 64),
#endif
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
    (const char[]) {0x09, 0x04}, // 0: is supported language is English (0x0409)
    "OrangeFox86",               // 1: Manufacturer
    "Dreamcast Controller USB",  // 2: Product
    NULL,                        // 3: Serial (special case; get pico serial)
    "P1",                        // 4: Gamepad 1
    "P2",                        // 5: Gamepad 2
    "P3",                        // 6: Gamepad 3
    "P4",                        // 7: Gamepad 4
    "MSC",                       // 8: Mass Storage Class
    "CDC",                       // 9: Communication Device Class
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    uint8_t chr_count;
    char buffer[32] = {0};

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // Convert ASCII string into UTF-16

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

        const char *str = string_desc_arr[index];

        if (str == NULL)
        {
            if (index == 3)
            {
                // Special case: try to get pico serial number
                pico_get_unique_board_id_string(buffer, sizeof(buffer));
                if (buffer[0] != '\0')
                {
                    str = buffer;
                }
                else
                {
                    // Something failed, have host assign serial
                    return NULL;
                }
            }
            else
            {
                return NULL;
            }
        }

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
