/*
 * bsp_uart.h
 *
 *  Created on: 2018Äê8ÔÂ17ÈÕ
 *      Author: ggg
 */

#ifndef BSP_BSP_UART_H_
#define BSP_BSP_UART_H_

#include <stdint.h>
extern void my_UART_open(void);
extern void my_UART_close(void);
extern int UART_send(const uint8_t *pui8Data, uint16_t ui16Size);
extern void UART_receive(uint8_t *pui8Buffer, uint16_t ui16Size);
extern void UART_flush(void);

#endif /* BSP_BSP_UART_H_ */
