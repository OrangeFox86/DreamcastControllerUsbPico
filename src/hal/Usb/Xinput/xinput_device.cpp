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
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"
#include "tusb_config.h"

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "device/usbd.h"
#include "device/usbd_pvt.h"

#include "xinput_device.hpp"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;        // optional Out endpoint

  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_EP_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_EP_BUFSIZE];
} xinput_interface_t;

CFG_TUSB_MEM_SECTION static xinput_interface_t _xinput_itf[CFG_TUD_XINPUT];

/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
	for (uint8_t i=0; i < CFG_TUD_XINPUT; i++ )
	{
		if ( itf_num == _xinput_itf[i].itf_num ) return i;
	}

	return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xinput_n_ready(uint8_t instance)
{
  uint8_t const ep_in = _xinput_itf[instance].ep_in;
  return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_xinput_n_report(uint8_t instance, uint8_t report_id, void const* report, uint8_t len)
{
  uint8_t const rhport = 0;
  xinput_interface_t * p_xinput = &_xinput_itf[instance];

  // claim endpoint
  TU_VERIFY( usbd_edpt_claim(rhport, p_xinput->ep_in) );

  // prepare data
  if (report_id)
  {
    len = tu_min8(len, CFG_TUD_XINPUT_EP_BUFSIZE-1);

    p_xinput->epin_buf[0] = report_id;
    p_xinput->epin_buf[1] = len;
    memcpy(p_xinput->epin_buf+2, report, len);
    len++;
  }else
  {
    // If report id = 0, skip ID and len fields
    len = tu_min8(len, CFG_TUD_XINPUT_EP_BUFSIZE);
    memcpy(p_xinput->epin_buf, report, len);
  }

  return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xinput->ep_in, p_xinput->epin_buf, len);
}

bool tud_xinput_n_gamepad_report(uint8_t instance, uint8_t report_id,
                                 uint16_t buttons, uint8_t lt, uint8_t rt, int lx, int ly, int rx, int ry)
{
  xinput_gamepad_report_t report =
  {
    .buttons    = buttons,
    .lt         = lt,
    .rt         = rt,
    .lx         = lx,
    .ly         = ly,
    .rx         = rx,
    .ry         = ry
  };

  return tud_xinput_n_report(instance, report_id, &report, sizeof(report));
}

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xinput_reset(uint8_t rhport)
{
  (void) rhport;
  tu_memclr(_xinput_itf, sizeof(_xinput_itf));
}

void xinput_init(void)
{
  xinput_reset(TUD_OPT_RHPORT);
}

uint16_t xinput_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len)
{
  //+16 is for the unknown descriptor
  uint16_t const drv_len = sizeof(tusb_desc_interface_t) + desc_itf->bNumEndpoints*sizeof(tusb_desc_endpoint_t) + 17;
  TU_VERIFY(max_len >= drv_len, 0);

  // Find available interface
  xinput_interface_t * p_xinput = NULL;
  uint8_t hid_id;
  for(hid_id=0; hid_id<CFG_TUD_XINPUT; hid_id++)
  {
    if ( _xinput_itf[hid_id].ep_in == 0 )
    {
      p_xinput = &_xinput_itf[hid_id];
      break;
    }
  }
  TU_ASSERT(p_xinput, 0);

  uint8_t const *p_desc = (uint8_t const *) desc_itf;

  //------------- Endpoint Descriptor -------------//
  p_desc = tu_desc_next(p_desc);
  TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, desc_itf->bNumEndpoints, TUSB_XFER_BULK, &p_xinput->ep_out, &p_xinput->ep_in), 0);

  // Prepare for output endpoint
  if (p_xinput->ep_out)
  {
    if ( !usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf, sizeof(p_xinput->epout_buf)) )
    {
      TU_LOG_FAILED();
      TU_BREAKPOINT();
    }
  }

  return drv_len;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool xinput_control_xfer_cb (uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  return true;
}

bool xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void) result;

  uint8_t instance = 0;
  xinput_interface_t * p_xinput = _xinput_itf;

  // Identify which interface to use
  for (instance = 0; instance < CFG_TUD_XINPUT; instance++)
  {
    p_xinput = &_xinput_itf[instance];
    if ( (ep_addr == p_xinput->ep_out) || (ep_addr == p_xinput->ep_in) ) break;
  }
  TU_ASSERT(instance < CFG_TUD_XINPUT);

  // Sent report successfully
  if (ep_addr == p_xinput->ep_in)
  {
    // TODO: add hook
  }
  // Received report
  else if (ep_addr == p_xinput->ep_out)
  {
    // TODO
    //tud_xinput_set_report_cb(instance, p_xinput->epout_buf, xferred_bytes);
    TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf, sizeof(p_xinput->epout_buf)));
  }

  return true;
}

static usbd_class_driver_t const xinput_driver =
{
  #if CFG_TUSB_DEBUG >= 2
    .name = "XINPUT",
#endif
    .init             = xinput_init,
    .reset            = xinput_reset,
    .open             = xinput_open,
    .control_xfer_cb  = xinput_control_xfer_cb,
    .xfer_cb          = xinput_xfer_cb,
    .sof              = NULL
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 1;
    return &xinput_driver;
}

#endif
