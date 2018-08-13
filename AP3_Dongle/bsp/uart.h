/*
 * uart.h
 *
 *  Created on: 2018Äê2ÔÂ28ÈÕ
 *      Author: ggg
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_
#include <stddef.h>
#include <stdint.h>
#include <ti/drivers/UART.h>

#define READ_BUF_LEN    64
#define READ_CMD_LEN    1

extern int32_t UART_appRead(void *buffer, size_t size);
extern int32_t UART_appWrite(void *buffer, size_t size);
extern void UART_appInit(void);

extern UART_Handle uart_handle;

#endif /* BSP_UART_H_ */
