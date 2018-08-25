/*
 * mySPI.h
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#ifndef BSP_APPSPI_H_
#define BSP_APPSPI_H_
#include <stdint.h>
#include <ti/drivers/SPI.h>

extern void SPI_appInit(uint8_t* rxbuf, uint8_t* txbuf);
extern uint16_t SPI_appRecv(void *buffer, uint16_t size);
extern uint16_t SPI_appSend(void *buffer, uint16_t size);
extern void SPI_bufferInit(uint8_t* rxbuf, uint8_t* txbuf);

extern SPI_Handle handle;
extern SPI_Transaction transaction;

#endif /* BSP_APPSPI_H_ */
