/*
 * SPI_private.h
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#ifndef PROTOCOL_SPI_PRIVATE_H_
#define PROTOCOL_SPI_PRIVATE_H_
#include <stdint.h>

#pragma pack(1)

typedef struct st_SPI_private{
    uint8_t sn;
    uint16_t len;
    void *buf;
    uint16_t crc;
}st_SPI_private;

#pragma pack()

extern void SPI_PrivateDataInit(void);

#endif /* PROTOCOL_SPI_PRIVATE_H_ */
