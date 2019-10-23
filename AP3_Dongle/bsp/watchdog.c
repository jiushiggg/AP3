/*
 * watchdog.c
 *
 *  Created on: 2018Äê12ÔÂ12ÈÕ
 *      Author: ggg
 */

#include <stdint.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Watchdog.h>
#include <ti/devices/cc26x0r2/driverlib/sys_ctrl.h>
#include "watchdog.h"
#include "Board.h"

Watchdog_Handle watchdogHandle;

void watchdogCallback(uintptr_t unused)
{
    /* Clear watchdog interrupt flag */
//    Watchdog_clear(watchdogHandle);

    SysCtrlSystemReset();

    /* Insert timeout handling code here. */
}

void watchdog_init(void)
{
    Watchdog_Params params;

    Watchdog_init();
    /* Create and enable a Watchdog with resets disabled */
    Watchdog_Params_init(&params);
    params.callbackFxn = (Watchdog_Callback)watchdogCallback;
    params.resetMode = Watchdog_RESET_ON;

    watchdogHandle = Watchdog_open(Board_WATCHDOG0, &params);
    if (watchdogHandle == NULL) {
        /* Error opening Watchdog */
        while (1);
    }
}

void watchdog_clear(void)
{
	Watchdog_clear(watchdogHandle);
}
