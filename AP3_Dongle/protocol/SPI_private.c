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
#include <ti/drivers/SPI.h>
#include "event.h"
#include "protocol.h"
#include "appSPI.h"
#include "core.h"
#include "crc16.h"
#include "debug.h"
#include "bsp_uart.h"
#include "flash.h"
#include "CC2592.h"

#define SPI_DATA		(uint16_t)(0X0000)
#define SPI_DATA_ERR     (uint16_t)(0X2000)
#define SPI_QUERY       (uint16_t)(0X4000)
#define SPI_ABORT		(uint16_t)(0X6000)
#define SPI_LAST_PCK    (uint16_t)(0X1000)
#define SPI_LEN_MASK    (uint16_t)(0X0FFF)
#define SPI_CMD_MASK    (uint16_t)(0XE000)

#define SPI_QUERY_SN   (uint8_t)(0)
#define SPI_QUERY_CMD   (uint8_t)(SPI_QUERY>>8)
#define SPI_NONE_CMD        (uint8_t)(0X00)

#define SPI_NO_USE      NULL

#define DATA_RIGHT			0
#define DATA_LEN_OVERFLOW 	1
#define DATA_LEN_ERR		2
#define DATA_CRC_ERR		3
#define DATA_NOT_STORE		4

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
.recvToFlashFnx =   SPIPrivate_recvToFlash,
.USCIStatusFnx	= 	SPI_checkStatus
};

typedef enum{
    CALCU_CRC = (uint8_t)0,
    UN_CALCU_CRC
}emCrcFlg;


typedef enum{
	ST_SPI_RECV_INIT = (uint8_t)0,
	ST_SPI_SEND_INIT,					//1
    ST_SPI_PACKET_TRANS_DATA,
    ST_SPI_WRITE_SEND_BUF,
    ST_SPI_SEND_DATA,
    ST_SPI_PACKET_CHECK_DATA,			//5
	ST_SPI_SET_RECV,
	ST_SPI_UNPACKED,
	ST_SPI_PACK_DATA,
	ST_SPI_RECV_PACKET,
	ST_SPI_DATA_HANDLE,					//10
	ST_SPI_QUERY_HANDLE,
	ST_SPI_ABORT_HANDLE,
	ST_SPI_ERR_HANDLE,
    ST_SPI_RECV_DATA,
    ST_SPI_END,							//15
	ST_SPI_ERR,
	ST_SPI_EXIT,
}emPrivateState;

#pragma pack(1)
typedef struct st_SPIProtocolCmd{
    st_SPI_privateHead head;
    uint16_t cmd;
    uint32_t len;
    union cmdData{
        uint16_t crc;
        struct PAenable{
            uint8_t PAFlg;
            uint16_t crc;
        }PAenable;
    }data;
}st_SPIProtocolCmd;
#pragma pack()

static sn_t spi_sn;
volatile static emPrivateState privateState = ST_SPI_EXIT;

#define MAX_PACKET_NUM		32	//32*4000/512/8
static uint8_t SPI_pkg_bitmap[MAX_PACKET_NUM] = {0};

void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t len)
{
    privateState = ST_SPI_EXIT;
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

    //Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex(dst, sizeof(st_SPI_privateHead)+tmp_len+sizeof(pckst->crc));
    Debug_SetLevel(DEBUG_LEVEL_INFO);

}

static uint16_t unpackData(uint32_t src, st_SPI_private* pckst)
{
    int16_t tmp_len = 0;
    memcpy((uint8_t*)pckst, (uint8_t*)src, sizeof(st_SPI_privateHead));					//copy head
    tmp_len = pckst->head.len&SPI_LEN_MASK;
    pckst->buf = ((uint8_t*)src + sizeof(st_SPI_privateHead));							//copy pointer

    memcpy((uint8_t*)&pckst->crc, (uint8_t*)pckst->buf + tmp_len, sizeof(pckst->crc));	//copy crc

    SPIP_DEBUG(("sn:%d, len:%x,crc:%x\r\n", pckst->head.sn, pckst->head.len, pckst->crc));

//    Debug_SetLevel(DEBUG_LEVEL_DEBUG);												//deubg
    pdebughex((uint8_t*)src, sizeof(st_SPI_privateHead)+tmp_len+sizeof(pckst->crc));
    Debug_SetLevel(DEBUG_LEVEL_INFO);

    return CRC16_CaculateStepByStep(0, (uint8_t*)src, sizeof(st_SPI_privateHead)+tmp_len);
}

//static emPrivateState packageSend(uint8_t* buf, uint16_t len, sn_t *x, uint32_t timeout)
//{
//    emPrivateState tmp_state = ST_SPI_ERR;
//    SPI_recCmdAckFlg = true;
//    SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
//    pdebughex(buf, len);
//
//    if (true==Device_Recv_pend(timeout)){
//        GPIO_write(Board_SPI_SLAVE_READY, 1);
//        x->send_retry_times = 0;
//        SPI_recCmdAckFlg = false;
//        tmp_state = ST_SPI_RECV_DATA;
//        SPIP_DEBUG(("sendOk"));
//    }else{
//        GPIO_write(Board_SPI_SLAVE_READY, 1);
//        SPIP_DEBUG(("sendTimeout"));
//        tmp_state = ST_SPI_SEND_DATA;
//        x->send_retry_times++;
//        if (x->send_retry_times>3){
//            x->send_retry_times = 0;
//            tmp_state = ST_SPI_ERR;
//            SPIP_DEBUG(("sendMax3\r\n"));
//            SPI_recCmdAckFlg = false;
//        }
//    }
//    return tmp_state;
//}
static Bool packetRecv(sn_t *x, uint32_t timeout)
{
	Bool ret = false;
    if (true==Device_Recv_pend(timeout)){
        x->send_retry_times = 0;
        SPIP_DEBUG(("receiveOk\r\n"));
        ret = true;
    }else{
        SPIP_DEBUG(("receiveTimeout\r\n"));
        x->send_retry_times++;
        ret = false;
    }
    GPIO_write(Board_SPI_SLAVE_READY, 1);

    return ret;
}

#define BITMAP_CLEARED	0
#define BITMAP_UNCLEAR	1
#define BITMAP_OVERFLOW	-1

static int8_t check_bitmap(uint16_t packet_num)
{
    uint16_t packet_byte = 0;;
    uint8_t packet_bit = 0;

    packet_byte = packet_num / 8;
    packet_bit  = (uint8_t)(packet_num % 8);

    if(packet_byte >= MAX_PACKET_NUM)
    {
        return BITMAP_OVERFLOW;
    }

    if((SPI_pkg_bitmap[packet_byte] & (0x01<<packet_bit)) == 0){
    	return BITMAP_CLEARED;
    }

    return BITMAP_UNCLEAR;
}

static int8_t bitmap_clear_bit(uint16_t packet_num)
{
    uint16_t packet_byte = 0;;
    uint8_t packet_bit = 0;

    packet_byte = packet_num / 8;
    packet_bit  = (uint8_t)(packet_num % 8);
	SPI_pkg_bitmap[packet_byte] &= ~(0x01<<packet_bit);

	return BITMAP_CLEARED;
}


static void SPIPrivate_end(uint8_t* buf_ptr, st_SPI_private * tmp)
{

    SPIP_DEBUG(("ST_SPI_END\r\n"));
    SPI_preSend();
    memset(buf_ptr, 0, SPIPRIVATE_LEN_ALL);
    tmp->head.sn = 0;
    tmp->head.len = SPI_LAST_PCK;
    packetData(buf_ptr, 0, tmp, CALCU_CRC);
    memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
    SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
}

static int32_t SPI_send(sn_t *x, uint32_t src, int32_t len, int32_t timeout, BOOL (*fnx)(uint32_t addr1, uint8_t* dst, uint32_t len1))
{
    st_SPI_private tmp;
    st_SPI_private send_check;
    uint8_t *tx_ptr = NULL;
    uint8_t *rx_ptr = NULL;
    uint32_t tmp_addr = src;
    uint16_t calu_crc;
    int32_t ret_len = 0;
    int16_t tmp_len = 0;
    uint16_t i=0;
    privateState = ST_SPI_SEND_INIT;

    while(privateState != ST_SPI_EXIT){
        switch (privateState){
            case ST_SPI_SEND_INIT:
                SPIP_DEBUG(("--->ST_SPI_SEND_INIT\r\n"));
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
                SPI_preSend();
                if (NULL == fnx){
                    packetData(tx_ptr, tmp_addr, &tmp, UN_CALCU_CRC);
                }else {
                    if(fnx(tmp_addr, tx_ptr+sizeof(st_SPI_privateHead), tmp_len) == FALSE)
                    {
                    	SPIP_DEBUG_ERR(("flash read error!"));
                        privateState = ST_SPI_ERR;
                        break;
                    }
                    packetData(tx_ptr, (uint32_t)(tx_ptr+sizeof(st_SPI_privateHead)), &tmp, CALCU_CRC);
                }
                privateState = ST_SPI_SEND_DATA;
                break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("--->ST_SPI_SEND_DATA\r\n"));
                //privateState = packageSend(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead), x, timeout);
                SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
            	privateState = ST_SPI_RECV_DATA;
                break;
            case ST_SPI_RECV_DATA:
                SPIP_DEBUG(("--->ST_SPI_RECV_DATA------cmd:%d\r\n", x->last_recv_cmd));
                if (false == packetRecv(x, timeout)){
                	privateState = x->send_retry_times<3 ? ST_SPI_WRITE_SEND_BUF : ST_SPI_ERR;
                	break;
                }
                calu_crc = unpackData((uint32_t)rx_ptr, &tmp);
                SPIP_DEBUG(("calu_crc:%x, send_sn:%d\r\n", calu_crc, x->send_sn));

                if (calu_crc==tmp.crc && *(uint16_t*)tmp.buf==CORE_CMD_BACK_TO_IDLE){
                	privateState = ST_SPI_WRITE_SEND_BUF;			//send data again
                	break;
                }

                if (x->last_recv_cmd == SPI_QUERY_CMD){
                    x->last_recv_cmd = SPI_NONE_CMD;

                    if ((tmp.head.len&SPI_DATA_ERR) == SPI_DATA_ERR){
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
                    	SPIP_DEBUG_ERR(("crcOrSnErr\r\n"));
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
                SPI_preSend();
                send_check.head.sn = x->send_sn;
                send_check.head.len = SPI_QUERY;
                packetData(tx_ptr, 0, &send_check, CALCU_CRC);
                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                privateState = ST_SPI_SEND_DATA;
                break;
            case ST_SPI_END:
                SPIPrivate_end(tx_ptr, &tmp);
                privateState = ST_SPI_EXIT;
                break;
            case ST_SPI_ERR:
            	SPIP_DEBUG_ERR(("->ST_SPI_ERR\r\n"));
                //no break;
            default:
                SPIPrivate_end(tx_ptr, &tmp);
                privateState = ST_SPI_EXIT;
                memset((uint8_t*)&x, 0, sizeof(sn_t));
                SPIP_DEBUG(("case default"));
                break;
        }
    }

    return ret_len;
}

static uint8_t checkData(st_SPI_private* pckst, uint32_t src)
{
    int16_t tmp_len = 0;
    uint16_t crc = 0;
    memcpy((uint8_t*)pckst, (uint8_t*)src, sizeof(st_SPI_privateHead));					//copy head
    tmp_len = pckst->head.len&SPI_LEN_MASK;

    if (tmp_len > SPIPRIVATE_LEN_DAT){
    	SPIP_DEBUG_ERR(("ERR tmp len=%d\r\n", tmp_len));
    	return DATA_LEN_OVERFLOW;
    }

    pckst->buf = ((uint8_t*)src + sizeof(st_SPI_privateHead));							//copy pointer

    memcpy((uint8_t*)&pckst->crc, (uint8_t*)pckst->buf + tmp_len, sizeof(pckst->crc));	//copy crc

    SPIP_DEBUG_REC(("sn:%d, len:%x,crc:%x\r\n", pckst->head.sn, pckst->head.len, pckst->crc));

    if (SPI_LAST_PCK==pckst->head.len){			//error data, because data length is 0.
    	SPIP_DEBUG_ERR(("ERR len is %d\r\n", pckst->head.len));
    	return DATA_LEN_ERR;
    }

    Debug_SetLevel(DEBUG_LEVEL_DEBUG);												//deubg
    pdebughex((uint8_t*)src, sizeof(st_SPI_privateHead));
    Debug_SetLevel(DEBUG_LEVEL_INFO);

    crc = CRC16_CaculateStepByStep(0, (uint8_t*)src, sizeof(st_SPI_privateHead)+tmp_len);
    if (pckst->crc != crc){
    	SPIP_DEBUG_ERR(("ERR crc:%x, %x\r\n", pckst->crc, crc));
    	return DATA_CRC_ERR;
    }
    return DATA_RIGHT;
}

static int32_t SPI_recv(sn_t *x, uint32_t addr, int32_t len, int32_t timeout, BOOL (*fnx)(uint32_t addr1, uint8_t* src1, uint32_t len1))
{
    st_SPI_private tmp;
    uint32_t tmp_buff_adr = 0;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint8_t ret_checkdata;
    int32_t total_len = 0;
    uint16_t tmp_len = 0;
    int8_t ret_bitmap = 0;
    uint16_t cmd;
    privateState = ST_SPI_RECV_INIT;

    while(privateState != ST_SPI_EXIT){
        switch (privateState){
            case ST_SPI_RECV_INIT:
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                memset(SPI_pkg_bitmap, 0xff, sizeof(SPI_pkg_bitmap));
                memset(x, 0, sizeof(sn_t));
                SPIP_DEBUG_REC(("->ST_SPI_RECV_INIT:len=%d, timeout=%d\r\n", len, timeout));
                privateState = ST_SPI_RECV_PACKET;
                break;
            case ST_SPI_RECV_PACKET:
                SPIP_DEBUG_REC(("->ST_SPI_RECV_PACKET %dms\r\n", timeout/100));
                if (false == packetRecv(x, timeout)){
                	SPIP_DEBUG_REC(("ERR recv Timeout\r\n"));
                	privateState = x->send_retry_times<3 ? ST_SPI_RECV_PACKET : ST_SPI_ERR;
                	break;
                }

                ret_checkdata = checkData(&tmp, (uint32_t)rx_ptr);
                if (0 != ret_checkdata){
                	SPIP_DEBUG_REC(("ERR ret_checkdata=%d\r\n", ret_checkdata));
                	privateState = ST_SPI_ERR_HANDLE;
                	break;
                }
                tmp_len = tmp.head.len & SPI_LEN_MASK;
                if (total_len + tmp_len > len){
                	SPIP_DEBUG_REC(("ERR total_len=%d, tmp_len=%d, len=%d\r\n", total_len, tmp_len, len));
                	privateState = ST_SPI_ERR_HANDLE;
                	break;
                }

                if (*(uint16_t*)tmp.buf == CORE_CMD_BACK_TO_IDLE){		//termination command
                	privateState = ST_SPI_END;
                	break;
                }

                cmd = tmp.head.len&SPI_CMD_MASK;
                if (tmp.head.sn != x->send_sn){
                	SPIP_DEBUG_ERR(("ERR packet sn=%d, sn=%d\r\n", tmp.head.sn, x->send_sn));
                	x->send_sn = tmp.head.sn;
                	privateState = ST_SPI_ERR_HANDLE;
                	break;
                }

                if (cmd == SPI_DATA){
                	privateState = ST_SPI_DATA_HANDLE;
                }else if (cmd == SPI_QUERY){
                	privateState = ST_SPI_QUERY_HANDLE;
                }else{
                	privateState = ST_SPI_ERR_HANDLE;
                }
            	break;
            case ST_SPI_DATA_HANDLE:
                SPIP_DEBUG_REC(("->ST_SPI_DATA_HANDLE\r\n"));
                ret_bitmap = BITMAP_UNCLEAR;
            	x->last_recv_cmd = tmp.head.len&SPI_LAST_PCK ? true : false;

            	ret_bitmap = check_bitmap(tmp.head.sn);
            	if (BITMAP_UNCLEAR == ret_bitmap){
            		x->nak_times = 0;
            		bitmap_clear_bit(tmp.head.sn);
    				if (NULL == fnx){
    					memcpy((uint8_t*)addr, tmp.buf, tmp_len);
    				}else {
    					if(fnx(addr, tmp.buf, tmp_len) == FALSE){
    						privateState = ST_SPI_ERR;
    						SPIP_DEBUG_REC(("ERR flash write error"));
    					}
    				}
    				addr += tmp_len;
    				total_len += tmp_len;
    				tmp_buff_adr = 0;
                    tmp.head.len = SPI_LAST_PCK;
                    privateState = ST_SPI_PACK_DATA;
            	}else if(BITMAP_CLEARED == ret_bitmap){
            		tmp_buff_adr = 0;
                    tmp.head.len = SPI_LAST_PCK;
                    privateState = ST_SPI_PACK_DATA;
            	}else{
            		SPIP_DEBUG_ERR(("ERR clear bitmap failed=%d\r\n", ret_bitmap));
                    privateState = ST_SPI_ERR;
            	}
            	break;
            case ST_SPI_QUERY_HANDLE:
                SPIP_DEBUG_REC(("->ST_SPI_QUERY_HANDLE\r\n"));
                if (BITMAP_CLEARED != check_bitmap(tmp.head.sn)){
                	SPIP_DEBUG_ERR(("WARN data not stored\r\n"));
                	privateState = ST_SPI_ERR_HANDLE;
                	ret_checkdata = DATA_NOT_STORE;
                	break;
                }
            	SPIP_DEBUG_REC(("sn add one\r\n"));
            	x->send_sn = tmp.head.sn+1;
				tmp_buff_adr = 0;
				tmp.head.len = SPI_LAST_PCK;
				privateState = x->last_recv_cmd==true ? ST_SPI_END : ST_SPI_PACK_DATA;
            	break;
            case ST_SPI_ERR_HANDLE:
            	SPIP_DEBUG_ERR(("->ST_SPI_ERR_HANDLE:%d\r\n", x->nak_times));
                if (++x->nak_times > 3){
                    x->nak_times = 0;
                    privateState = ST_SPI_ERR;
                    break;
                }
                tmp.head.sn = x->send_sn;
                tmp.head.len = SPI_LAST_PCK;
                tmp.head.len |= (ret_checkdata!=DATA_RIGHT ? SPI_DATA_ERR : SPI_DATA);
                tmp_buff_adr = 0;
                privateState = ST_SPI_PACK_DATA;
            	break;
            case ST_SPI_PACK_DATA:
            	SPIP_DEBUG_REC(("->ST_SPI_PACK_DATA\r\n"));
            	SPI_preSend();
            	packetData(tx_ptr, tmp_buff_adr, &tmp, CALCU_CRC);
            	privateState = ST_SPI_SEND_DATA;
            	break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG_REC(("->ST_SPI_SEND_DATA\r\n"));
                SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
            	privateState = ST_SPI_RECV_PACKET;
                break;
            case ST_SPI_END:
            	SPIP_DEBUG_REC(("->ST_SPI_END\r\n"));
                SPIPrivate_end(tx_ptr, &tmp);
                privateState = ST_SPI_EXIT;
                break;
            case ST_SPI_ERR:
            	SPIP_DEBUG_ERR(("->ST_SPI_ERR\r\n"));
                //no break;
            default:
                SPIPrivate_end(tx_ptr, &tmp);
                privateState = ST_SPI_EXIT;
                SPIP_DEBUG_REC(("default\r\n"));
                total_len = 0;
                break;
        }
    }
    return total_len;
}

int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("-------------SPIPrivate_send: len=%d---\r\n", len));
    //Debug_SetLevel(DEBUG_LEVEL_DEBUG);
    pdebughex(src, len);
    //Debug_SetLevel(DEBUG_LEVEL_INFO);
    ret_len = SPI_send(x, (uint32_t)src, len, timeout, NULL);
    SPIP_DEBUG(("---SPIPrivate_send exit---:%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG(("-------------SPIPrivate_sendFromFlash:addr=%x,len=%d---\r\n", addr, len));
    ret_len = SPI_send(x, addr, len, timeout, Flash_Read);
    SPIP_DEBUG(("------SPIPrivate_sendFromFlash exit------:%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t len)
{
    int32_t ret_len = 0;
    SPIP_DEBUG_REC(("-------------SPIPrivate_recv---\r\n"));
    memset((uint8_t*)&spi_sn, 0, sizeof(sn_t));
    ret_len = SPI_recv(&spi_sn, (uint32_t)read_buf, len, EVENT_WAIT_US(500000), NULL);
    SPIP_DEBUG_REC(("---SPIPrivate_recv exit---%d\r\n", ret_len));
    return ret_len;
}

int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout)
{
    int32_t ret_len = 0;
    SPIP_DEBUG_REC(("---SPIPrivate_recvToFlash---\r\n"));
    ret_len = SPI_recv(x, addr, dst_len, timeout, Flash_Write);
    SPIP_DEBUG_REC(("---SPIPrivate_recvToFlash exit---%d\r\n", ret_len));
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
    st_SPIProtocolCmd *ptr = (st_SPIProtocolCmd*)trans->rxBuf;

    if ((trans->status)==SPI_TRANSFER_CSN_DEASSERT
        || (trans->status)==SPI_TRANSFER_COMPLETED){
		if (ptr->cmd==CORE_CMD_BACK_TO_IDLE && ptr->len==0 && ptr->head.len==0x1006 && ptr->head.sn==0){	 //没有计算CRC，简单的判断长度。6=data_cmd+data_len，0x1000表示最后一包
			core_idel_flag = 1;
	        if (privateState==ST_SPI_EXIT){
				SPI_preSend();
	        	SPI_appSend(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
	            SPIP_DEBUG(("1:%d\r\n", privateState));
	        }else{
	        	Device_Recv_post();
	        	SPIP_DEBUG(("3:%d\r\n", privateState));
	        }
		}else if (privateState<ST_SPI_EXIT){
			Device_Recv_post();
			SPIP_DEBUG(("2:%d\r\n", privateState));
		}else{
			Event_Set(EVENT_COMMUNICATE_RX_HANDLE);
			Device_Recv_post();
			//SPIP_DEBUG(("4444\r\n"));
			SPIP_DEBUG(("4:%d\r\n", privateState));
		}
    }else if (trans->status==SPI_TRANSFER_CANCELED && privateState!=ST_SPI_SEND_INIT){
    	SPI_appRecv(SPI_NO_USE, SPIPRIVATE_LEN_ALL);
    	SPIP_DEBUG(("6:%d\r\n", privateState));
    }else {
    	SPIP_DEBUG(("7:%d\r\n", privateState));
    }
}
