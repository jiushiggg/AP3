/*
 * SPI_private.c
 *
 *  Created on: 2018年8月13日
 *      Author: ggg
 *      private protocol
 *      SN(1Byte) | LEN(2Byte) | Data(0-512) | CRC(2Byte)
 *      SN：接收的数据包号。
 *      LEN:length。最后一包数据由bit12位置1（0x1000）表示。
 *      Data:data
 *      CRC: sn+len+data的CRC。
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Board.h"
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/knl/Task.h>  //debug
#include "event.h"
#include "protocol.h"
#include "appSPI.h"
#include "core.h"
#include "crc16.h"
#include "debug.h"
#include "bsp_uart.h"
#include "flash.h"

#define SPI_CRC_ERR     (uint16_t)(0X2000)
#define SPI_QUERY       (uint16_t)(0X4000)
#define SPI_LAST_PCK    (uint16_t)(0X1000)
#define SPI_LEN_MASK    (uint16_t)(0X0FFF)

#define SPI_QUERY_SN   (uint8_t)(0)
#define SPI_QUERY_CMD   (uint8_t)(SPI_QUERY>>8)
#define SPI_NONE_CMD        (uint8_t)(0X00)

#define SPI_NO_USE      NULL


void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t tmp_len);
int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t tmp_len);
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

typedef enum{
    CALCU_CRC = (uint8_t)0,
    UN_CALCU_CRC
}emCrcFlg;


typedef enum{
    ST_SPI_INIT = (uint8_t)0,
    ST_SPI_PACKET_TRANS_DATA,
    ST_SPI_WRITE_SEND_BUF,
    ST_SPI_SEND_DATA,
    ST_SPI_SEND_LAST_DATA,
    ST_SPI_PACKET_CHECK_DATA,
    ST_SPI_RECV_DATA,
    ST_SPI_RECV_NEXT_DATA,
    ST_SPI_RECV_LAST_DATA,
    ST_SPI_ERR,
    ST_SPI_END,

}emPrivateState;

static sn_t spi_sn;
static volatile Bool SPI_writeFlashFlg   = false;
static volatile Bool SPI_recCmdAckFlg   = false;
static volatile  INT16  spi_recv_len_once = 0;
static emPrivateState privateState = ST_SPI_INIT;

void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t len)
{
    privateState = ST_SPI_INIT;
    memset((uint8_t*)&spi_sn, 0, sizeof(sn_t));
    SPIP_DEBUG(("SPIPrivate_dataInit\r\n"));
}

static void packetData(uint8_t* dst, uint32_t src, st_SPI_private* pckst, emCrcFlg flg)
{
    int16_t tmp_len = pckst->head.len&0x0fff;
    memcpy(dst, (uint8_t*)pckst, sizeof(st_SPI_privateHead));

    if (0!=src && src!=(uint32_t)(dst+sizeof(st_SPI_privateHead))){
        memcpy(dst+sizeof(st_SPI_privateHead), (uint8_t*)src, tmp_len);
    }

    if (CALCU_CRC == flg){
        pckst->crc = CRC16_CaculateStepByStep(0, dst, sizeof(st_SPI_privateHead)+tmp_len);
    }

    memcpy(dst+sizeof(st_SPI_privateHead)+tmp_len, (uint8_t*)&pckst->crc, sizeof(pckst->crc));

//    Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex(dst, sizeof(st_SPI_privateHead)+tmp_len+sizeof(pckst->crc));
    Debug_SetLevel(DEBUG_LEVEL_INFO);

}

static uint16_t unpackData(uint32_t src, st_SPI_private* pckst)
{
    int16_t tmp_len = 0;
    memcpy((uint8_t*)pckst, (uint8_t*)src, sizeof(st_SPI_privateHead));
    tmp_len = pckst->head.len&SPI_LEN_MASK;
    pckst->buf = ((uint8_t*)src + sizeof(st_SPI_privateHead));

    memcpy((uint8_t*)&pckst->crc, (uint8_t*)pckst->buf + tmp_len, sizeof(pckst->crc));

    SPIP_DEBUG(("sn:%d, len:%x,crc:%x\r\n", pckst->head.sn, pckst->head.len, pckst->crc));

//    Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex((uint8_t*)src, sizeof(st_SPI_privateHead)+tmp_len+sizeof(pckst->crc));
    Debug_SetLevel(DEBUG_LEVEL_INFO);

    return CRC16_CaculateStepByStep(0, (uint8_t*)src, sizeof(st_SPI_privateHead)+tmp_len);
}

static emPrivateState packageSend(uint8_t* buf, uint16_t len, sn_t *x, uint32_t timeout)
{
    emPrivateState tmp_state = ST_SPI_ERR;
    SPI_recCmdAckFlg = true;
    SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
    pdebughex(buf, len);

    if (true==Device_Recv_pend(timeout)){
        GPIO_write(Board_SPI_SLAVE_READY, 1);
        x->send_retry_times = 0;
        SPI_recCmdAckFlg = false;
        tmp_state = ST_SPI_RECV_DATA;
        SPIP_DEBUG(("sendOk"));
    }else{
        GPIO_write(Board_SPI_SLAVE_READY, 1);
        SPIP_DEBUG(("sendTimeout"));
        tmp_state = ST_SPI_SEND_DATA;
        x->send_retry_times++;
        if (x->send_retry_times>3){
            x->send_retry_times = 0;
            tmp_state = ST_SPI_ERR;
            SPIP_DEBUG(("sendMax3\r\n"));
            SPI_recCmdAckFlg = false;
        }
    }
    return tmp_state;
}


static void SPIPrivate_end(uint8_t* buf_ptr, st_SPI_private * tmp)
{

    SPIP_DEBUG(("ST_SPI_END\r\n"));
    tmp->head.sn = 0;
    tmp->head.len = SPI_LAST_PCK;
    packetData(buf_ptr, 0, tmp, CALCU_CRC);
    memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));

    SPI_appRecv(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
}

static int32_t SPI_send(sn_t *x, uint32_t src, int32_t len, int32_t timeout, BOOL (*fnx)(uint32_t addr1, uint8_t* dst, uint32_t len1))
{
    st_SPI_private tmp;
    st_SPI_private send_check;
    uint8_t *tx_ptr = NULL;
    uint8_t *rx_ptr = NULL;
    uint32_t tmp_addr = src;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;
    int16_t tmp_len = 0;
    uint16_t i=0;

    while(false == exit_flg){
        switch (privateState){
            case ST_SPI_INIT:
                SPIP_DEBUG(("--->ST_SPI_WRITE_SEND_BUF\r\n"));
                SPI_cancle();
                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                x->last_recv_cmd = SPI_NONE_CMD;
                transaction.count = SPIPRIVATE_LEN_ALL;
                tx_ptr = transaction.txBuf = spi_send_buf;
                rx_ptr = transaction.rxBuf = recv_once_buf;
                privateState = ST_SPI_PACKET_TRANS_DATA;
                break;
            case ST_SPI_PACKET_TRANS_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_TRANS_DATA------%d times\r\n", i++));

                tmp.head.sn = x->send_sn;
                if (len > SPIPRIVATE_LEN_DAT){
                    tmp.head.len = SPIPRIVATE_LEN_DAT;
                    ret_len += tmp.head.len;
                    len -= SPIPRIVATE_LEN_DAT;
                }else{
                    tmp.head.len = len > 0 ? len : 0;
                    ret_len += tmp.head.len;
                    len -= tmp.head.len;
                    tmp.head.len |= SPI_LAST_PCK;
                }
                SPIP_DEBUG(("ret_len=%d,tmp.headlen=%d,len=%d\r\n", ret_len, tmp.head.len, len));
                tmp_len = tmp.head.len & SPI_LEN_MASK;
                tmp_addr = src;

                if (NULL == fnx){
                    calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
                    tmp.crc = CRC16_CaculateStepByStep(calu_crc,  (uint8_t*)tmp_addr, tmp_len);
                }

                src += tmp_len;
                SPIP_DEBUG(("head sn：%d,len:%d,tmp_addr:%x\r\n", tmp.head.sn, tmp_len, tmp_addr));
                privateState = ST_SPI_WRITE_SEND_BUF;
                break;
            case ST_SPI_WRITE_SEND_BUF:
                SPIP_DEBUG(("--->ST_SPI_WRITE_SEND_BUF\r\n"));
                if (NULL == fnx){
                    packetData(tx_ptr, tmp_addr, &tmp, UN_CALCU_CRC);
                }else {
                    if(fnx(tmp_addr, tx_ptr+sizeof(st_SPI_privateHead), tmp_len) == FALSE)
                    {
                        SPIP_DEBUG(("flash read error!"));
                        privateState = ST_SPI_ERR;
                        break;
                    }
                    packetData(tx_ptr, (uint32_t)(tx_ptr+sizeof(st_SPI_privateHead)), &tmp, CALCU_CRC);
                }
                privateState = ST_SPI_SEND_DATA;
                break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_SEND_DATA\r\n"));
                privateState = packageSend(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead), x, timeout);
                break;
            case ST_SPI_RECV_DATA:
                SPIP_DEBUG(("--->ST_SPI_RECV_DATA------cmd:%d\r\n", x->last_recv_cmd));
                calu_crc = unpackData((uint32_t)rx_ptr, &tmp);
                SPIP_DEBUG(("calu_crc:%x, send_sn:%d\r\n", calu_crc, x->send_sn));

                if (x->last_recv_cmd == SPI_QUERY_CMD){
                    x->last_recv_cmd = SPI_NONE_CMD;

                    if ((tmp.head.len&SPI_CRC_ERR) == SPI_CRC_ERR){
                        privateState = ++x->nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                        SPIP_DEBUG(("dataCRCErr\r\n"));
                    }else if (calu_crc==tmp.crc && tmp.head.sn==x->send_sn){
                        x->nak_times = 0;
                        SPIP_DEBUG(("crcRight\r\n"));
                        if (len > 0){
                            privateState = ST_SPI_PACKET_TRANS_DATA;
                            x->send_sn++;
                            SPIP_DEBUG(("send next\r\n"));
                        }else{
                            privateState = ST_SPI_END;
                            SPIP_DEBUG(("goToEnd\r\n"));
                        }
                    }else{
                        SPIP_DEBUG(("crcOrSnErr\r\n"));
                        privateState = ++x->nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                    }
                }else{
                    SPIP_DEBUG(("goToCheck\r\n"));
                    privateState = ST_SPI_PACKET_CHECK_DATA;
                }
                break;
            case ST_SPI_PACKET_CHECK_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_CHECK_DATA\r\n"));
                x->last_recv_cmd = SPI_QUERY_CMD;

                send_check.head.sn = x->send_sn;
                send_check.head.len = SPI_QUERY;
                packetData(tx_ptr, 0, &send_check, CALCU_CRC);
                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                privateState = ST_SPI_SEND_DATA;
                break;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            case ST_SPI_ERR:
                SPIP_DEBUG(("->ST_SPI_ERR\r\n"));
                //no break;
            default:
                privateState = ST_SPI_INIT;
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                memset((uint8_t*)&x, 0, sizeof(sn_t));
                SPIP_DEBUG(("case default"));
                break;
        }
    }

    return ret_len;
}

static int32_t SPI_recv(sn_t *x, uint32_t addr, int32_t len, int32_t timeout, BOOL (*fnx)(uint32_t addr1, uint8_t* src1, uint32_t len1))
{
    st_SPI_private tmp;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;

    while(false == exit_flg){

        switch (privateState){
            case ST_SPI_INIT:
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                privateState = ST_SPI_RECV_DATA;
                SPIP_DEBUG(("InitExit%d\r\n", exit_flg));
                break;
            case ST_SPI_RECV_DATA:
            {
                int32_t tmp_len = 0;
                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));

                calu_crc = unpackData((uint32_t)rx_ptr, &tmp);
                tmp_len = tmp.head.len & SPI_LEN_MASK;

                pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                SPIP_DEBUG(("spi_sn=%x, calu=%x\r\n", x->send_sn, calu_crc));

                if (tmp.crc == calu_crc && x->send_sn==tmp.head.sn){
                    x->nak_times = 0;
                    if (ret_len+tmp_len > len){
                        privateState = ST_SPI_ERR;
                        SPIP_DEBUG(("lenErr\r\n"));
                    } else if(tmp.head.len == SPI_QUERY){
                        x->send_sn = tmp.head.sn+1;
                        privateState = x->last_recv_cmd==true ? ST_SPI_END : ST_SPI_SEND_DATA;
                        SPIP_DEBUG(("query\r\n"));
                    }else{                                      //right packet
                        if (NULL == fnx){
                            memcpy((uint8_t*)addr, tmp.buf, tmp_len);
                        }else {
//                            GGGDEBUG(("recv addr:%x,tmp_len:%d\r\n", addr, tmp_len));
//                            Debug_SetLevel(DEBUG_LEVEL_DEBUG);
//                            pdebughex(tmp.buf, tmp_len);
//                            Debug_SetLevel(DEBUG_LEVEL_INFO);

                            if(fnx(addr, tmp.buf, tmp_len) == FALSE)
                            {
                                privateState = ST_SPI_ERR;
                                SPIP_DEBUG(("flash write error"));
                            }
                        }
                        addr += tmp_len;
                        ret_len += tmp_len;
                        x->last_recv_cmd = tmp.head.len&SPI_LAST_PCK ? true : false;
                        SPIP_DEBUG(("recvData:%d,ret_len:%d.", x->last_recv_cmd, ret_len));
                    }
                    tmp.head.len = SPI_LAST_PCK;
                }else{
                    tmp.head.len = SPI_LAST_PCK;
                    if (tmp.crc != calu_crc){
                        tmp.head.len |= SPI_CRC_ERR;
                    }

                    if (++x->nak_times > 3){
                        x->nak_times = 0;
                        privateState = ST_SPI_ERR;
                    }
                    SPIP_DEBUG(("recvErr"));
                    pdebughex(tx_ptr, sizeof(st_SPI_privateHead)+sizeof(calu_crc));
                }

                if (ST_SPI_END!=privateState && ST_SPI_ERR!=privateState){
                    packetData(tx_ptr, 0, &tmp, CALCU_CRC);
                    privateState = ST_SPI_SEND_DATA;
                }
                break;
            }
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
                privateState = packageSend(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead), x, timeout);
                break;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            case ST_SPI_ERR:
                SPIP_DEBUG(("->ST_SPI_ERR\r\n"));
                //no break;
            default:
                privateState = ST_SPI_INIT;
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                SPIP_DEBUG(("default\r\n"));
                ret_len = 0;
                break;
        }
    }
    return ret_len;
}

int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("-------------SPIPrivate_send: len=%d---\r\n", len));
    //Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex(src, len);
    //Debug_SetLevel(DEBUG_LEVEL_INFO);
    ret_len = SPI_send(x, (uint32_t)src, len, EVENT_WAIT_US(1000000), NULL);
    SPIP_DEBUG(("---SPIPrivate_send exit---:%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("-------------SPIPrivate_sendFromFlash:addr=%x,len=%d---\r\n", addr, len));
    ret_len = SPI_send(x, addr, len, EVENT_WAIT_US(1000000), Flash_Read);
    SPIP_DEBUG(("------SPIPrivate_sendFromFlash exit------:%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t len)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("-------------SPIPrivate_recv---\r\n"));
    ret_len = SPI_recv(&spi_sn, (uint32_t)read_buf, len, EVENT_WAIT_US(1000000), NULL);
    SPIP_DEBUG(("---SPIPrivate_recv exit---%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("---SPIPrivate_recvToFlash---\r\n"));
    ret_len = SPI_recv(x, addr, dst_len, EVENT_WAIT_US(1000000), Flash_Write);
    SPIP_DEBUG(("---SPIPrivate_recvToFlash exit---%d\r\n", ret_len));
    return ret_len;
}

uint8_t *SPIPrivate_getData(uint32_t *len)
{
    SPIP_DEBUG(("SPIPrivate_getData\r\n"));
    return 0;
}
extern volatile uint8_t core_idel_flag;
void transferCallback(SPI_Handle handle, SPI_Transaction *trans)
{
    uint8_t *ptr=trans->rxBuf;

    if ((ptr[sizeof(st_SPI_privateHead)] | (uint16_t)ptr[sizeof(st_SPI_privateHead)+1]<<8)==CORE_CMD_BACK_TO_IDLE && SPIPRIVATE_LEN_ALL==trans->count){
        core_idel_flag = 1;
        SPIP_DEBUG(("1111\r\n"));
    }else if (SPI_recCmdAckFlg == true && SPIPRIVATE_LEN_ALL==trans->count){
        Device_Recv_post();
        SPIP_DEBUG(("2222\r\n"));
    }else if(SPIPRIVATE_LEN_ALL==trans->count && SPI_writeFlashFlg == true){
        Device_Recv_post();
        SPIP_DEBUG(("3333\r\n"));
    }else if (SPIPRIVATE_LEN_ALL==trans->count){
        Event_communicateSet(EVENT_COMMUNICATE_RX_HANDLE);
        SPIP_DEBUG(("4444\r\n"));
    }else{
        memset(trans->rxBuf, 0, trans->count);
        SPIP_DEBUG(("5555cnt%d,f:%d,a:%d\r\n", trans->count, SPI_writeFlashFlg,SPI_recCmdAckFlg));
        if (privateState!=ST_SPI_INIT){
            SPI_appRecv(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
        }
    }
    spi_recv_len_once = trans->count;
}


