#ifndef _WAKEUP_H_
#define _WAKEUP_H_

#include "datatype.h"

INT32 wakeup_start(UINT32 addr, UINT32 len, UINT8 type);
INT32 wakeup_get_loop_times(UINT32 addr);
extern INT32 set_wakeup_led_flash(UINT32 addr, void *buf, UINT32 len);
#endif
