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

typedef struct st_SPI_privateHead{
    uint8_t sn;
    uint16_t len;
}st_SPI_privateHead;


typedef struct st_SPI_private{
    st_SPI_privateHead head;
    void *buf;
    uint16_t crc;
}st_SPI_private;


#pragma pack()

#define SPIPRIVATE_LEN_SN           1
#define SPIPRIVATE_LEN_LEN          2
#define SPIPRIVATE_LEN_DAT          512
#define SPIPRIVATE_LEN_CRC          2
#define SPIPRIVATE_LEN_ALL          (SPIPRIVATE_LEN_SN+SPIPRIVATE_LEN_LEN+SPIPRIVATE_LEN_DAT+SPIPRIVATE_LEN_CRC)


#endif /* PROTOCOL_SPI_PRIVATE_H_ */
