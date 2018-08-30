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
    ST_SPI_INIT = (uint8_t)0,
    ST_SPI_PACKET_SEND_DATA,
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

void packetData(uint8_t* dst, uint8_t* src, st_SPI_private* pckst)
{

}

static void SPIPrivate_end(uint8_t* buf_ptr, st_SPI_private * tmp)
{

    SPIP_DEBUG(("ST_SPI_END\r\n"));
//    tmp->head.sn = 0;
//    tmp->head.len = SPI_LAST_PCK;
//    tmp->crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
//    memcpy(buf_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
//    memcpy(buf_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&tmp->crc, sizeof(tmp->crc));
    memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
    GPIO_write(Board_SPI_SLAVE_READY, 1);

//    pdebughex(tx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
    SPI_appRecv(SPI_NO_USE, 517);
    SPIP_DEBUG(("endExit\r\n"));
}

int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout)
{
    st_SPI_private tmp;
    st_SPI_privateHead send_check;
    uint8_t *tx_ptr = NULL;
    uint8_t *rx_ptr = NULL;
    uint8_t *tmp_ptr = src;
    uint16_t calu_crc;
//    uint8_t *send_ptr = src;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;
    int16_t tmp_len = 0;
    uint16_t i=0;
    SPIP_DEBUG(("-------------SPIPrivate_send: len=%d---\r\n", len));
    //Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex(src, len);
    //Debug_SetLevel(DEBUG_LEVEL_INFO);
    while(false == exit_flg){
        switch (privateState){
            case ST_SPI_INIT:
                SPI_cancle();

                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                x->last_recv_cmd = SPI_NONE_CMD;
                transaction.count = 517;
                tx_ptr = transaction.txBuf = spi_send_buf;
                rx_ptr = transaction.rxBuf = recv_once_buf;
                privateState = ST_SPI_PACKET_SEND_DATA;
                SPIP_DEBUG(("initExit\r\n"));
                break;
            case ST_SPI_PACKET_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_SEND_DATA------%d times\r\n", i++));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
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
                tmp_ptr = src;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
                tmp.crc = CRC16_CaculateStepByStep(calu_crc,  tmp_ptr, tmp_len);

                src += tmp_len;
                SPIP_DEBUG(("head sn：%d, len:%d\r\n", tmp.head.sn, tmp_len));
                privateState = ST_SPI_WRITE_SEND_BUF;
                SPIP_DEBUG(("packetDataExit\r\n"));
                break;
            case ST_SPI_WRITE_SEND_BUF:
                tmp_len = tmp.head.len&0x0fff;
                SPIP_DEBUG(("--->ST_SPI_WRITE_SEND_BUF\r\n"));
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), tmp_ptr, tmp_len);
                memcpy(tx_ptr+sizeof(st_SPI_privateHead)+tmp_len, (uint8_t*)&tmp.crc, sizeof(tmp.crc));

                pdebughex(tx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("writeBufExit\r\n"));
                break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;

                SPI_appSend(NULL, 517);
                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    spi_sn.send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk\r\n"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout\r\n"));
                    if (++spi_sn.send_retry_times > 3){
                        spi_sn.send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3\r\n"));
                    }
                }
                SPIP_DEBUG(("sendDataExit\r\n"));
                break;
            case ST_SPI_RECV_DATA:
                SPIP_DEBUG(("--->ST_SPI_RECV_DATA------cmd:%d\r\n", x->last_recv_cmd));
                {
                    uint8_t tmp_len = 0;
    //                SPI_appRecv(SPI_NO_USE, 517);
                    memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                    tmp_len = tmp.head.len&SPI_LEN_MASK;
                    SPIP_DEBUG(("tmp.head.len:%d, tmp_len:%d\r\n", tmp.head.len, tmp_len));
                    tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                    SPIP_DEBUG(("rx:%x, buf:%x\r\n", rx_ptr, tmp.buf));
                    memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + tmp_len, sizeof(tmp.crc));
                    SPIP_DEBUG(("tmp.crc:%x\r\n", tmp.crc));
                    calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+tmp_len);

                    Debug_SetLevel(DEBUG_LEVEL_DEBUG);
                    pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                    Debug_SetLevel(DEBUG_LEVEL_INFO);

                    if (x->last_recv_cmd == SPI_QUERY_CMD){
                        x->last_recv_cmd = SPI_NONE_CMD;
                        memset((uint8_t*)&send_check, 0, sizeof(send_check));
                        if ((tmp.head.len&SPI_CRC_ERR) == SPI_CRC_ERR){
                            privateState = ++spi_sn.nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                            SPIP_DEBUG(("dataCRCErr\r\n"));
                        }else if (calu_crc==tmp.crc && tmp.head.sn==x->send_sn){
                            spi_sn.nak_times = 0;
                            SPIP_DEBUG(("crcRight\r\n"));
                            if (len > 0){
                                privateState = ST_SPI_PACKET_SEND_DATA;
                                x->send_sn++;
                                SPIP_DEBUG(("send next\r\n"));
                            }else{
                                privateState = ST_SPI_END;
                                SPIP_DEBUG(("goToEnd\r\n"));
                            }
                        }else{
                            SPIP_DEBUG(("calu_crc:%x,tmp.crc:%d,tmp.headsn:%d,send_sn:%d\r\n", calu_crc, tmp.crc, tmp.head.sn, x->send_sn));
                            SPIP_DEBUG(("crcOrSnErr\r\n"));
                            privateState = ++spi_sn.nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                        }
                    }else{
                        SPIP_DEBUG(("goToCheck\r\n"));
                        privateState = ST_SPI_PACKET_CHECK_DATA;
                    }

                }
                SPIP_DEBUG(("recvDataExit\r\n"));
                break;
            case ST_SPI_PACKET_CHECK_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_CHECK_DATA\r\n"));
                x->last_recv_cmd = SPI_QUERY_CMD;
                send_check.sn = 0;
                send_check.len = SPI_QUERY;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&send_check , sizeof(st_SPI_privateHead));
                memcpy(tx_ptr, (uint8_t*)&send_check, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
                Debug_SetLevel(DEBUG_LEVEL_DEBUG);
                pdebughex(tx_ptr, sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                Debug_SetLevel(DEBUG_LEVEL_INFO);
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("packetCheckDataExit\r\n"));
                break;
            case ST_SPI_ERR:
                SPIP_DEBUG(("ST_SPI_ERR\r\n"));
                ret_len = 0;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            default:
                privateState = ST_SPI_INIT;
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                memset((uint8_t*)&x, 0, sizeof(sn_t));
                SPIP_DEBUG(("case default"));
                break;
        }
    }
    SPIP_DEBUG(("---SPIPrivate_send exit---:%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t len)
{
    st_SPI_private tmp;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;

    SPIP_DEBUG(("-------------SPIPrivate_recv---\r\n"));
    while(false == exit_flg){

        switch (privateState){
            case ST_SPI_INIT:
                memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
                privateState = ST_SPI_RECV_DATA;
                SPIP_DEBUG(("InitExit%d\r\n", exit_flg));
                break;
//            case ST_SPI_RECV_NEXT_DATA:
//                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));
//                SPI_recCmdAckFlg = true;
//                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
//                    spi_sn.send_retry_times = 0;
//                    SPI_recCmdAckFlg = false;
//                    privateState = ST_SPI_RECV_DATA;
//                }else{
//                    SPI_recCmdAckFlg = false;
//                }
//                SPIP_DEBUG(("recvNextDataExit\r\n"));
                //no break;
            case ST_SPI_RECV_DATA:
            {
                int32_t tmp_len = 0;
                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                tmp_len = tmp.head.len&SPI_LEN_MASK;
                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + (tmp_len), sizeof(tmp.crc));
                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+tmp_len);
                pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                SPIP_DEBUG(("calu=%x, buf=%x", calu_crc, tmp.crc));
                SPIP_DEBUG(("spi_sn=%x, tmp_sn=%x", spi_sn.send_sn, tmp.head.sn));
                if (tmp.crc == calu_crc && spi_sn.send_sn==tmp.head.sn){
                    spi_sn.nak_times = 0;
                    if (ret_len+tmp_len > len){
                        privateState = ST_SPI_ERR;
                        SPIP_DEBUG(("lenErr"));
                    } else if(tmp.head.len == SPI_QUERY){
                        spi_sn.send_sn = tmp.head.sn+1;
                        privateState = spi_sn.last_recv_cmd==true ? ST_SPI_END : ST_SPI_SEND_DATA;
                        SPIP_DEBUG(("query"));
                    }else{                                      //right packet
                        memcpy(read_buf, tmp.buf, tmp_len);
                        spi_sn.last_recv_cmd = tmp.head.len&SPI_LAST_PCK ? true : false;
                        ret_len += tmp_len;
                        tmp.head.len = SPI_LAST_PCK;
                        SPIP_DEBUG(("recvData:%d", spi_sn.last_recv_cmd));
                    }                    
                }else{
                    tmp.head.len = SPI_LAST_PCK;
                    if (tmp.crc != calu_crc){
                        tmp.head.len |= SPI_CRC_ERR;
                    }

                    if (++spi_sn.nak_times > 3){
                        spi_sn.nak_times = 0;
                        privateState = ST_SPI_ERR;
                    }
                    SPIP_DEBUG(("recvErr"));
                    pdebughex(tx_ptr, sizeof(st_SPI_privateHead)+sizeof(calu_crc));
                }

                if (ST_SPI_END!=privateState && ST_SPI_ERR!=privateState){
                    tx_ptr[0] = tmp.head.sn;
                    memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                    calu_crc = CRC16_CaculateStepByStep(0, tx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
                    memcpy(tx_ptr + sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
                    privateState = ST_SPI_SEND_DATA;
                }

                SPIP_DEBUG(("recvDataExit\r\n"));
                break;
            }
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;
                SPI_appSend(SPI_NO_USE, 517);
                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));

                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    spi_sn.send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout"));
                    if (++spi_sn.send_retry_times > 3){
                        spi_sn.send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3"));
                    }
                }

                SPIP_DEBUG(("sendDataExit\r\n"));
                break;
            case ST_SPI_ERR:
                ret_len = 0;
                //no break;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            default:
                privateState = ST_SPI_INIT;
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                SPIP_DEBUG(("default\r\n"));
                len = 0;
                break;
        }

    }
    SPIP_DEBUG(("---SPIPrivate_recv exit---%d\r\n", exit_flg));
    return ret_len;
}

uint8_t *SPIPrivate_getData(uint32_t *len)
{
    SPIP_DEBUG(("SPIPrivate_getData\r\n"));
    return 0;
}

int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout)
{
    st_SPI_private tmp;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;

    SPIP_DEBUG(("---SPIPrivate_recvToFlash---\r\n"));
    while(false == exit_flg){

        switch (privateState){
            case ST_SPI_INIT:
                memset((uint8_t*)x, 0, sizeof(sn_t));
                privateState = ST_SPI_RECV_DATA;
                SPIP_DEBUG(("InitExit%d\r\n", exit_flg));
                break;
            case ST_SPI_RECV_DATA:
            {
                int32_t tmp_len = 0;
                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                tmp_len = tmp.head.len&SPI_LEN_MASK;
                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + (tmp_len), sizeof(tmp.crc));
                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+tmp_len);
                pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                SPIP_DEBUG(("calu=%x, buf=%x", calu_crc, tmp.crc));
                SPIP_DEBUG(("x->sn=%x, tmp_sn=%x", x->send_sn, tmp.head.sn));

                if (tmp.crc==calu_crc && (x->send_sn==tmp.head.sn || SPI_QUERY_SN==tmp.head.sn)){
                    x->nak_times = 0;
                    if (ret_len+tmp_len > dst_len){
                        privateState = ST_SPI_ERR;
                        SPIP_DEBUG(("lenErr"));
                    }else if(tmp.head.len == SPI_QUERY){
                        x->send_sn = tmp.head.sn+1;
                        privateState = x->last_recv_cmd==true ? ST_SPI_END : ST_SPI_SEND_DATA;
                        SPIP_DEBUG(("query"));
                    }else{
                        SPIP_DEBUG(("addr=%x,addr_rx=%x,len=%d", addr, tmp.buf, tmp_len));
                        if(Flash_Write(addr, tmp.buf, tmp_len) == FALSE)
                        {
                            privateState = ST_SPI_ERR;
                            SPIP_DEBUG(("flash write error"));
                        }
                        addr += tmp_len;
                        ret_len += tmp_len;
                        x->last_recv_cmd = tmp.head.len&SPI_LAST_PCK ? true : false;
                        SPIP_DEBUG(("recvData:%d,ret_len:%d", x->last_recv_cmd, ret_len));
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
                    tx_ptr[0] = tmp.head.sn;
                    memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                    calu_crc = CRC16_CaculateStepByStep(0, tx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
                    memcpy(tx_ptr + sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));

                    pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                    privateState = ST_SPI_SEND_DATA;
                }
                SPIP_DEBUG(("recvDataExit\r\n"));
                break;
            }
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;
                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                SPI_appSend(SPI_NO_USE, 517);
                
                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    x->send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout"));
                    if (++x->send_retry_times > 3){
                        x->send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3"));
                    }
                }

                SPIP_DEBUG(("sendDataExit\r\n"));
                break;
            case ST_SPI_ERR:
                ret_len = 0;
                //no break;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            default:
                SPIPrivate_end(tx_ptr, &tmp);
                privateState = ST_SPI_INIT;
                exit_flg = true;
                SPIP_DEBUG(("default\r\n"));
                dst_len = 0;
                break;
        }

    }
    SPIP_DEBUG(("---SPIPrivate_recvToFlash exit---%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    st_SPI_private tmp;
    st_SPI_privateHead send_check;
    uint8_t *tx_ptr = NULL;
    uint8_t *rx_ptr = NULL;
    uint16_t calu_crc;
    int32_t  tmp_addr;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;
    int16_t tmp_len = 0;
    uint16_t i=0;
    SPIP_DEBUG(("-------------SPIPrivate_sendFromFlash: len=%d---\r\n", len));
    //Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    //pdebughex(src, len);
    //Debug_SetLevel(DEBUG_LEVEL_INFO);
    while(false == exit_flg){
        switch (privateState){
            case ST_SPI_INIT:
                SPI_cancle();

                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                x->last_recv_cmd = SPI_NONE_CMD;
                transaction.count = 517;
                tx_ptr = transaction.txBuf = spi_send_buf;
                rx_ptr = transaction.rxBuf = recv_once_buf;
                privateState = ST_SPI_PACKET_SEND_DATA;
                SPIP_DEBUG(("initExit\r\n"));
                break;
            case ST_SPI_PACKET_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_SEND_DATA------%d times\r\n", i++));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
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
                SPIP_DEBUG(("ret_len=%x,tmp.headlen=%d,len=%x\r\n", ret_len, tmp.head.len, len));
                tmp_len = tmp.head.len & SPI_LEN_MASK;
                tmp_addr = addr;
                if(Flash_Read(tmp_addr, tx_ptr+sizeof(st_SPI_privateHead), tmp_len) == FALSE)
                {
                    X_DEBUG(("flash read error!"));
                    break;
                }
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                tmp.crc = CRC16_CaculateStepByStep(0, tx_ptr , tmp_len+sizeof(st_SPI_privateHead));

                Debug_SetLevel(DEBUG_LEVEL_DEBUG);
                pdebughex(tx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                Debug_SetLevel(DEBUG_LEVEL_INFO);

                addr += tmp_len;
                SPIP_DEBUG(("head sn:%d, len:%d ", tmp.head.sn, tmp_len));
                privateState = ST_SPI_WRITE_SEND_BUF;
                SPIP_DEBUG(("packetDataExit\r\n"));
                break;
            case ST_SPI_WRITE_SEND_BUF:
                tmp_len = tmp.head.len&0x0fff;
                SPIP_DEBUG(("--->ST_SPI_WRITE_SEND_BUF\r\n"));
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                if(Flash_Read(tmp_addr, tx_ptr+sizeof(st_SPI_privateHead), tmp_len) == FALSE)
                {
                    SPIP_DEBUG(("flash read error!"));
                    privateState = ST_SPI_ERR;
                    break;
                }
                memcpy(tx_ptr+sizeof(st_SPI_privateHead)+tmp_len, (uint8_t*)&tmp.crc, sizeof(tmp.crc));

                pdebughex(tx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("writeBufExit\r\n"));
                break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;

                SPI_appSend(NULL, 517);
                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    spi_sn.send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk\r\n"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout\r\n"));
                    if (++spi_sn.send_retry_times > 3){
                        spi_sn.send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3\r\n"));
                    }
                }
                SPIP_DEBUG(("sendDataExit\r\n"));
                break;
            case ST_SPI_RECV_DATA:
                SPIP_DEBUG(("--->ST_SPI_RECV_DATA------cmd:%d\r\n", x->last_recv_cmd));
                {
                    uint8_t tmp_len = 0;
    //                SPI_appRecv(SPI_NO_USE, 517);
                    memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                    tmp_len = tmp.head.len&SPI_LEN_MASK;
                    SPIP_DEBUG(("tmp.head.len:%x, tmp_len:%x\r\n", tmp.head.len, tmp_len));
                    tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                    SPIP_DEBUG(("rx:%x, buf:%x\r\n", rx_ptr, tmp.buf));
                    memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + tmp_len, sizeof(tmp.crc));
                    calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+tmp_len);
                    SPIP_DEBUG(("tmp.crc:%x, calu_crc:%x\r\n", tmp.crc, calu_crc));

                    Debug_SetLevel(DEBUG_LEVEL_DEBUG);
                    pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                    Debug_SetLevel(DEBUG_LEVEL_INFO);

                    if (x->last_recv_cmd == SPI_QUERY_CMD){
                        x->last_recv_cmd = SPI_NONE_CMD;
                        memset((uint8_t*)&send_check, 0, sizeof(send_check));
                        if ((tmp.head.len&SPI_CRC_ERR) == SPI_CRC_ERR){
                            privateState = ++spi_sn.nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                            SPIP_DEBUG(("dataCRCErr\r\n"));
                        }else if (calu_crc==tmp.crc && tmp.head.sn==x->send_sn){
                            spi_sn.nak_times = 0;
                            SPIP_DEBUG(("crcRight\r\n"));
                            if (len > 0){
                                privateState = ST_SPI_PACKET_SEND_DATA;
                                x->send_sn++;
                                SPIP_DEBUG(("send next\r\n"));
                            }else{
                                privateState = ST_SPI_END;
                                SPIP_DEBUG(("goToEnd\r\n"));
                            }
                        }else{
                            SPIP_DEBUG(("calu_crc:%x,tmp.crc:%d,tmp.headsn:%d,send_sn:%d\r\n", calu_crc, tmp.crc, tmp.head.sn, x->send_sn));
                            SPIP_DEBUG(("crcOrSnErr\r\n"));
                            privateState = ++spi_sn.nak_times > 3 ? ST_SPI_ERR : ST_SPI_WRITE_SEND_BUF;
                        }
                    }else{
                        SPIP_DEBUG(("goToCheck\r\n"));
                        privateState = ST_SPI_PACKET_CHECK_DATA;
                    }

                }
                SPIP_DEBUG(("recvDataExit\r\n"));
                break;
            case ST_SPI_PACKET_CHECK_DATA:
                SPIP_DEBUG(("--->ST_SPI_PACKET_CHECK_DATA\r\n"));
                x->last_recv_cmd = SPI_QUERY_CMD;
                send_check.sn = 0;
                send_check.len = SPI_QUERY;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&send_check , sizeof(st_SPI_privateHead));
                memcpy(tx_ptr, (uint8_t*)&send_check, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
                Debug_SetLevel(DEBUG_LEVEL_DEBUG);
                pdebughex(tx_ptr, sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                Debug_SetLevel(DEBUG_LEVEL_INFO);
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("packetCheckDataExit\r\n"));
                break;
            case ST_SPI_ERR:
                SPIP_DEBUG(("ST_SPI_ERR\r\n"));
                ret_len = 0;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                privateState = ST_SPI_INIT;
                break;
            default:
                privateState = ST_SPI_INIT;
                SPIPrivate_end(tx_ptr, &tmp);
                exit_flg = true;
                memset((uint8_t*)&x, 0, sizeof(sn_t));
                SPIP_DEBUG(("case default"));
                break;
        }
    }
    SPIP_DEBUG(("------SPIPrivate_sendFromFlash exit------:%d\r\n", ret_len));
    return ret_len;
}

extern volatile uint8_t core_idel_flag;
void transferCallback(SPI_Handle handle, SPI_Transaction *trans)
{
    uint8_t *ptr=trans->rxBuf;

    if ((ptr[2] | (uint16_t)ptr[3]<<8)==CORE_CMD_BACK_TO_IDLE && SPIPRIVATE_LEN_ALL==trans->count){
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
    }
    spi_recv_len_once = trans->count;
//    SPI_transfer(handle, trans);
}


