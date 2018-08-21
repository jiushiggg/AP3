/*
 * protocol.c
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#include <stddef.h>
#include "core.h"
#include "sys_cfg.h"
#include "protocol.h"
#include "crc16.h"


#pragma location = (TRANS_BUF_ADDR)
uint8_t recv_once_buf[TRANS_BUF_SIZE] = {0};          //the buffer used for UART receiving data

extern st_protocolFnxTable xmodemFnx;
extern st_protocolFnxTable SPIPrivateFnx;


st_protocolConfig protocolConfig[PROTOCOL_NUM] = {
{
.protocolFnxPtr = &SPIPrivateFnx,
},
{
 .protocolFnxPtr = &xmodemFnx,
}

};

void protocol_dataInit(uint8_t* tmp_buf, uint16_t tmp_len)
{
    protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->dataInitFnx(tmp_buf, tmp_len);
}

void Xmodem_reset(sn_t *x);


uint8_t *protocol_getData(UINT32 *len)
{
    return protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->getDataFnx(len);
}

int32_t protocol_recv(uint8_t* tmp_buf, uint16_t tmp_len)
{
    return protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->recvFnx(tmp_buf, tmp_len);
}

int32_t protocol_recvToFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    return protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->recvToFlashFnx(x, addr, len, timeout);
}


int32_t protocol_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    return protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->sendFromFlashFnx(x, addr, len, timeout);
}

int32_t protocol_send(sn_t *x, UINT8 *src, INT32 len, INT32 timeout)
{
    return protocolConfig[PROTOCOL_TYPE].protocolFnxPtr->sendFnx(x, src, len, timeout);
}

uint8_t protocol_checkCrc(void *buf, em_protocol type)
{
    void *tmp = NULL;
    uint16_t crc_buf, crc_calc = 0;
    uint16_t len;
    switch(type){
        case PROTOCOL_SPI:
            crc_buf  = ((st_SPI_private*)buf)->crc;
            crc_calc = CRC16_CaculateStepByStep(crc_calc, buf, offsetof(st_SPI_private, buf));
            tmp = ((st_SPI_private*)buf)->buf;
            len = ((st_SPI_private*)buf)->len;
            break;
        case PROTOCOL_XMODEM:
            break;
        default:
            break;
    }

    crc_calc = CRC16_CaculateStepByStep(crc_calc, tmp, len);

    return (crc_buf != crc_calc ? 0 : 1);
}





