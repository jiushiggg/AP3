/*
 * protocol.h
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#ifndef PROTOCOL_PROTOCOL_H_
#define PROTOCOL_PROTOCOL_H_

#include <stdint.h>

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
typedef void    (*PROT_dataInitFnx)(void);
typedef int32_t (*PROT_sendFnx)(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
typedef int32_t (*PROT_recvFnx)(void);
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


extern void protocol_dataInit(void);
extern uint8_t *protocol_getData(uint32_t *len);
extern int32_t protocol_recv(void);
extern int32_t protocol_recvToFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);
extern int32_t protocol_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);
extern int32_t protocol_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
extern uint8_t protocol_checkCrc(void *buf, em_protocol type);


#endif /* PROTOCOL_PROTOCOL_H_ */
