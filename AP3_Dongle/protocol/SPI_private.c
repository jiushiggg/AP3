/*
 * SPI_private.c
 *
 *  Created on: 2018年8月13日
 *      Author: ggg
 *      private protocol
 *      SN(1Byte) | LEN(2Byte) | Data(0-512) | CRC(2Byte)
 *      SN：子板成功接收数据后，SN号加1。返回给母板，母板用SN号组下一包数据。
 *      LEN:length。最后一包数据由bit12位置1（0x1000）表示。
 *      Data:data
 *      CRC: sn+len+data的CRC。
 *
 */
#include <stdint.h>
#include "SPI_private.h"
#include "protocol.h"
#include "appSPI.h"

void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t tmp_len);
int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
int32_t SPIPrivate_recv(uint8_t* tmp_buf, uint16_t tmp_len);
uint8_t *SPIPrivate_getData(uint32_t *len);
int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout);
int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);

st_protocolFnxTable SPIPrivateFnx={
.dataInitFnx    =   SPIPrivate_dataInit,
.sendFnx        =   SPIPrivate_send,
.recvFnx        =   SPIPrivate_recv,
.getDataFnx     =   SPIPrivate_getData,
.sendFromFlashFnx = SPIPrivate_sendFromFlash,
.recvToFlashFnx =   SPIPrivate_recvToFlash
};


void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t len)
{

}

int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout)
{

    return 0;
}

int32_t SPIPrivate_recv(uint8_t* tmp_buf, uint16_t len)
{
    return 0;
}

uint8_t *SPIPrivate_getData(uint32_t *len)
{
    return 0;
}

int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout)
{
    return 0;
}

int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    return 0;
}


void transferCallback(SPI_Handle handle, SPI_Transaction *transaction)
{

    SPI_transfer(handle, transaction);
}


