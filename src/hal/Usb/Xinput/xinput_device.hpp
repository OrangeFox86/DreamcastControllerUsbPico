#ifndef __XINPUT_DEVICE_H__
#define __XINPUT_DEVICE_H__

#include <stdint.h>

#include "common/tusb_common.h"

typedef struct TU_ATTR_PACKED
{
	uint16_t buttons;
	uint8_t lt;
	uint8_t rt;
	int lx;
	int ly;
	int rx;
	int ry;
} xinput_gamepad_report_t;

bool tud_xinput_n_ready(uint8_t instance);

bool tud_xinput_n_report(uint8_t instance, uint8_t report_id, void const* report, uint8_t len);

bool tud_xinput_n_gamepad_report(uint8_t instance, uint8_t report_id,
                                 uint16_t buttons, uint8_t lt, uint8_t rt, int lx, int ly, int rx, int ry);

void tud_xinput_set_report_cb(uint8_t instance, const uint8_t* buf, uint32_t xferred_bytes);


#endif // __XINPUT_DEVICE_H__
