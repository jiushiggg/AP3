/*
 * watchdog.h
 *
 *  Created on: 2018Äê12ÔÂ12ÈÕ
 *      Author: ggg
 */

#ifndef BSP_WATCHDOG_H_
#define BSP_WATCHDOG_H_

#define WD_RESET_TIME_S 	200
#define LOAD_TIME_S(n)	(n*1000)

extern void watchdog_init(void);
extern void watchdog_clear(void);

#endif /* BSP_WATCHDOG_H_ */
