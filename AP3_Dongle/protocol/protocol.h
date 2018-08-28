/*
 * protocol.h
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#ifndef PROTOCOL_PROTOCOL_H_
#define PROTOCOL_PROTOCOL_H_

#include <stdint.h>
#include "sys_cfg.h"
#include "SPI_private.h"
#include "xmodem.h"

typedef enum {
    PROTOCOL_SPI    = (uint8_t)0,
    PROTOCOL_XMODEM = (uint8_t)1,
    PROTOCOL_NUM    = (uint8_t)2
}em_protocol;

typedef struct sn_t{
    uint8_t last_recv_cmd;
    uint8_t last_recv_sn;
    uint8_t send_sn;
    int32_t send_retry_times;
    int32_t nak_times;
}sn_t;

typedef void    (*PROT_dataResetFnx)(sn_t *x);
typedef void    (*PROT_dataInitFnx)(uint8_t* tmp_buf, uint16_t len);
typedef int32_t (*PROT_sendFnx)(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
typedef int32_t (*PROT_recvFnx)(uint8_t* tmp_buf, uint16_t tmp_len);
typedef uint8_t* (*PROT_getDataFnx)(uint32_t* len);
typedef int32_t (*PROT_recvToFlashFnx)(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout);
typedef int32_t (*PROT_sendFromFlashFnx)(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);

typedef struct st_protocolFnxTable{
    PROT_dataInitFnx       dataInitFnx;
    PROT_sendFnx           sendFnx;
    PROT_recvFnx           recvFnx;
    PROT_getDataFnx        getDataFnx;
    PROT_sendFromFlashFnx  sendFromFlashFnx;
    PROT_recvToFlashFnx    recvToFlashFnx;
}st_protocolFnxTable;

typedef struct st_protocolConfig{
    st_protocolFnxTable const *protocolFnxPtr;
    void* rxBuf;
    void* txBuf;
    uint16_t bufLen;
}st_protocolConfig;


#if defined(PCIE)
    #define TRANS_BUF_SIZE  XMODEM_LEN_ALL
#elif defined(AP_3)
    #define TRANS_BUF_SIZE  SPIPRIVATE_LEN_ALL
#else
#endif
extern uint8_t recv_once_buf[TRANS_BUF_SIZE];
extern uint8_t spi_send_buf[TRANS_BUF_SIZE];


extern void protocol_dataInit(uint8_t* tmp_buf, uint16_t tmp_len);
extern uint8_t *protocol_getData(uint32_t *len);
extern int32_t protocol_recv(uint8_t* tmp_buf, uint16_t tmp_len);
extern int32_t protocol_recvToFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);
extern int32_t protocol_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);
extern int32_t protocol_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
extern uint8_t protocol_checkCrc(void *buf, em_protocol type);
extern void protocol_peripheralInit(void);


#endif /* PROTOCOL_PROTOCOL_H_ */
