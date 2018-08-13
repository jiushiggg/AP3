/*
 * communicate.h
 *
 *  Created on: 2018Äê3ÔÂ6ÈÕ
 *      Author: ggg
 */

#ifndef APP_COMMUNICATE_H_
#define APP_COMMUNICATE_H_

extern void communicate_main(void);
extern void TIM_SetSoftInterrupt(uint8_t enable, void (*p)(void));
extern void TIM_SoftInterrupt(void);
extern void TIM_ClearSoftInterrupt(void);
extern uint8_t Core_SendCmd(uint16_t cmd, uint32_t cmd_len, uint8_t *cmd_data);

#endif /* APP_COMMUNICATE_H_ */
