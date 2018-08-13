
#include <string.h>
#include <stdio.h>

#include <ti/sysbios/BIOS.h>

#include "debug.h"
#include "xmodem.h"
#include "crc16.h"
#include "flash.h"
#include "device.h"
#include "event.h"
#include "core.h"
#include "bsp.h"
#include "uart.h"




#define XMODEM_CMD_DFT			0X00
#define XMODEM_CMD_SOH			0X01
#define XMODEM_CMD_EOT			0X04
#define XMODEM_CMD_ACK			0X06
#define XMODEM_CMD_NAK			0X15
#define XMODEM_CMD_CAN			0X18


#define XMODEM_OFFSET_BAS		0
#define XMODEM_OFFSET_CMD		XMODEM_OFFSET_BAS
#define XMODEM_OFFSET_SN		(XMODEM_OFFSET_BAS+XMODEM_LEN_CMD)
#define XMODEM_OFFSET_DAT		(XMODEM_OFFSET_BAS+XMODEM_LEN_CMD+XMODEM_LEN_SN)
#define XMODEM_OFFSET_CRC		(XMODEM_OFFSET_BAS+XMODEM_LEN_CMD+XMODEM_LEN_SN+XMODEM_LEN_DAT)	

//#define TIMEOUT_RECV				10
#define RETRYTIME_TX			    3
#define RETRYTIME_NAK				3

static volatile Bool recCmdAckFlg    =   false;
static volatile Bool writeFlashFlg   = false;
static xmodem_t xcb;
#pragma location = (XCB_BUF_ADDR);
UINT8 xcb_buf[XCB_BUF_SIZE] = {0};                  //protocol buffer
INT32 xcb_recv_len = 0;                             //valid data length
static volatile  INT32  xcb_recv_len_once = 0;
#pragma location = (XMODEM_LEN_ALL_ADDR)
UINT8 recv_once_buf[XMODEM_LEN_ALL] = {0};          //the buffer used for UART receiving data

/*
** xmodem function
*/
static UINT8 Xmodem_CheckCrc(UINT8 *xbuf)
{
	UINT16 crc1, crc2;

	memcpy((void *)&crc1, xbuf+XMODEM_OFFSET_CRC, sizeof(crc1));
	crc2 = Crc16_Cal(xbuf, XMODEM_LEN_ALL-XMODEM_LEN_CRC);

	if(crc1 != crc2)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

//UINT8 Xmodem_RecvCmd(INT32 dev, INT32 timeout)
//{
//	UINT8 cmd = 0;
//	INT32 recv_len = 0;
//
//	if(Device_Select(dev, timeout) > 0)
//	{
//		recv_len = Device_Recv(dev, &cmd, sizeof(cmd), timeout);
//		if(recv_len != sizeof(cmd))
//		{
//			cmd = 0;
//		}
//	}
//
//	X_DEBUG((">>>Xmodem_RecvCmd:\r\nrecv_len = %d, cmd = 0x%02X.\r\n", recv_len, cmd));
//
//	return cmd;
//}

INT32 Xmodem_SendCmd(INT32 dev, UINT8 cmd, UINT8 recv_ack_flag, INT32 timeout)
{
	INT32 retry_time = RETRYTIME_TX;
	INT32 send_len = 0;
	UINT8 recv_ack = 0;
	INT32 read_len = 0;
	INT32 send_ret = 0;

	X_DEBUG((">en1,cmd=0x%02X,flag=%d,", cmd, recv_ack_flag));
	while(retry_time > 0)
	{	
		send_ret = Device_Send(dev, &cmd, sizeof(cmd), timeout);
		X_DEBUG(("send len:%d", send_ret));
		if(send_ret != sizeof(cmd))
		{
			retry_time--;
			continue;
		}

		if(recv_ack_flag == 0)
		{
			send_len += 1;
			break;
		}
	
		if(Device_Select(dev, timeout) <= 0)
		{
			retry_time--;
			continue;
		}

		recCmdAckFlg = true;
		if (true == Device_Recv_pend(EVENT_WAIT_US(1000000))){
	        read_len = xcb_recv_len_once;
	        recv_ack = recv_once_buf[0];
		}else {
		    read_len = 0;
		}
		recCmdAckFlg = false;

		if(read_len != sizeof(recv_ack))
		{
			retry_time--;
			continue;
		}
		
		if(recv_ack == XMODEM_CMD_ACK)
		{
			send_len += 1;
			break;
		}
		else if(recv_ack == XMODEM_CMD_CAN)
		{
			break;
		}
		else //XMODEM_CMD_CAN or other, cancl this send loop
		{
			retry_time--;
			continue;
		}
	}
	X_DEBUG(("ex1\r\n"));

	return send_len;
}


//ret 
INT32 Xmodem_RecvOnce(xmodem_t *x, INT32 dev, UINT8 **dst, INT32 timeout)
{
    INT32 ret = 0;
    UINT8 tx_cmd = XMODEM_CMD_NAK;
//  INT32 recv_len = 0;
//  INT32 copy_len = 0;

    X_DEBUG((">en2:"));

//  memset(recv_once_buf, 0, XMODEM_LEN_ALL);
//  recv_len = Device_Recv(dev, recv_once_buf, sizeof(recv_once_buf), timeout);

    X_DEBUG(("len=%d,cmd=0x%02X,sn=%d,lastsn=%d, ", xcb_recv_len_once, recv_once_buf[0], recv_once_buf[1], x->last_recv_sn));
//  pdebughex(recv_once_buf, sizeof(recv_once_buf));


    if(xcb_recv_len_once==XMODEM_LEN_ALL)
    {
        /* check crc */
        BSP_GPIO_ToggleDebugPin();
        if(Xmodem_CheckCrc(recv_once_buf) == 0)
        {
            X_DEBUG(("crc check error!"));
            goto recv_tx_ack;
        }
        BSP_GPIO_ToggleDebugPin();

        /* check cmd */
        if(recv_once_buf[0] != XMODEM_CMD_SOH)
        {
            X_DEBUG(("cmd check error!"));
            goto recv_tx_ack;
        }

        /* check sn */
        if(recv_once_buf[1] == (UINT8)(x->last_recv_sn+1))
        {
            x->last_recv_cmd = recv_once_buf[0];
            x->last_recv_sn += 1;
            tx_cmd = XMODEM_CMD_ACK;
            //recv_len = recv_len > len ? len : recv_len;
//          copy_len = XMODEM_LEN_DAT > len ? len : XMODEM_LEN_DAT;
            *dst = recv_once_buf + XMODEM_OFFSET_DAT;
//          memcpy(dst, recv_buf+XMODEM_OFFSET_DAT, copy_len);
            ret = XMODEM_LEN_DAT;
        }
        else if(recv_once_buf[1] == x->last_recv_sn)
        {
            X_DEBUG(("Same Pkg len:%d,CMD:%d", xcb_recv_len_once, recv_once_buf[0]));
            tx_cmd = XMODEM_CMD_ACK;
            goto recv_tx_ack;
        }
        else //total wrong sn
        {
            X_DEBUG(("sn error!"));
            goto recv_tx_ack;
        }
    }
    else if(xcb_recv_len_once==XMODEM_LEN_CMD)
    {
        X_DEBUG(("cmd:0x%02X, ", recv_once_buf[0]));
        if(recv_once_buf[0] == XMODEM_CMD_EOT)
        {
            tx_cmd = XMODEM_CMD_ACK;
            x->last_recv_cmd = XMODEM_CMD_EOT;
//          BSP_Delay1MS(30);
            goto recv_tx_ack;
        }
    }

recv_tx_ack:
    if(tx_cmd == XMODEM_CMD_NAK)
    {
        if((++x->nak_times) >= RETRYTIME_NAK)
        {
            tx_cmd = XMODEM_CMD_CAN;
            x->nak_times = 0;
            ret = -1;
            GGGDEBUG(("RET:-1"));
        }
    }
    else //ack
    {
        x->nak_times = 0;
    }

    //todo
    X_DEBUG(("\r\n>"));
    Xmodem_SendCmd(dev, tx_cmd, 0, timeout);
    X_DEBUG(("ex2\r\n"));
    return ret;
}
/*
 ** tx functions
 */
static INT32 Xmodem_MakePkg(UINT8 cmd, UINT8 sn, UINT8  *src, UINT16 src_len, \
								UINT8 *dst, UINT16 dst_len)
{
	INT32 ret = 0;
	UINT16 crc = 0;

	if(src_len > XMODEM_LEN_DAT)
	{
		ret = -1;
		goto done;
	}
	
	memset(dst, 0, dst_len);
	dst[0] = cmd;
	dst[1] = sn;
	memcpy(dst+XMODEM_OFFSET_DAT, src, src_len);
	crc = Crc16_Cal(dst, XMODEM_LEN_ALL-XMODEM_LEN_CRC);
	memcpy(dst+XMODEM_OFFSET_CRC, &crc, XMODEM_LEN_CRC);

	ret = XMODEM_LEN_ALL;
done:
	return ret;
}

INT32 Xmodem_SendOnce(xmodem_t *x, INT32 dev, UINT8 *src, INT32 len, INT32 timeout)
{
	INT32 ret = 0;
	INT32 send_len = 0;
	UINT8 send_buf[XMODEM_LEN_ALL] = {0};
	INT32 retry_time = RETRYTIME_TX;
	UINT8 recv_ack = 0;

	X_DEBUG((">en3:"));
	send_len = len > XMODEM_LEN_DAT ? XMODEM_LEN_DAT : len;
	Xmodem_MakePkg(XMODEM_CMD_SOH, x->send_sn, src, send_len, send_buf, sizeof(send_buf));
	X_DEBUG(("sn=%d,len=%d,cmd=0x%02X", x->send_sn, send_len, XMODEM_CMD_SOH));
	/* tx soh,sig,stx */
	while((retry_time--) > 0)
	{	
		if(Device_Send(dev, send_buf, sizeof(send_buf), timeout) != sizeof(send_buf))
		{
			continue;
		}

		//recv_ack = Xmodem_RecvCmd(dev, timeout);
        recCmdAckFlg = true;
        X_DEBUG(("pend "));
        if (true == Device_Recv_pend(EVENT_WAIT_US(1000000))){
            recv_ack = recv_once_buf[0];
        }else {
            recv_ack = XMODEM_CMD_NAK;
        }
        recCmdAckFlg = false;

		X_DEBUG(("ack: 0x%02X.", recv_ack));
		if(recv_ack == XMODEM_CMD_ACK)
		{
			x->send_sn += 1;
			ret = send_len;
			break;
		}
		else if(recv_ack == XMODEM_CMD_CAN)
		{
			ret = -1;
			break;
		}
		else // "NAK" or other, retry
		{
			ret = 0;
			continue;
		}
	}

	if(retry_time <= 0)
	{
		ret = -1;
	}
	
	X_DEBUG(("ex3\r\n"));
	return ret;
}

INT32 Xmodem_Send(xmodem_t *x, INT32 dev, UINT8 *src, INT32 len, INT32 timeout)
{
	UINT8 *ptr = src;
	INT32 left_len = len;
	INT32 send_len_once = 0;
	INT32 send_len_total = 0;

	X_DEBUG((">en4:"));
	/* init send */
	x->send_sn = 1;
	/* send data */
	while(left_len > 0)
	{	
		send_len_once = Xmodem_SendOnce(x, dev, ptr, left_len, timeout);
		if(send_len_once > 0) //ack
		{
			ptr += send_len_once;
			left_len -= send_len_once;
			send_len_total += send_len_once;
			send_len_once = 0;
		}
		else if(send_len_once == 0) // nak or other
		{
			continue;
		}
		else // "NAK" or other, retry
		{
			send_len_total = 0;
			break;
		}
	}
	
	/* all data has been sent, send eot */
	if(left_len <= 0)
	{
	    X_DEBUG(("\r\n>"));
		if(Xmodem_SendCmd(dev, XMODEM_CMD_EOT, 1, timeout) != 1)
		{
			send_len_total = 0;
		}
	}
	X_DEBUG(("ex4\r\n"));
	return send_len_total;
}

void Xmodem_Reset(xmodem_t *x)
{
	memset(x, 0, sizeof(xmodem_t));
}


void Xmodem_InitCallback(void)
{
	memset(&xcb, 0 , sizeof(xmodem_t));
	memset(xcb_buf, 0, sizeof(xcb_buf));
	xcb_recv_len = 0;
}

INT32 Xmodem_RecvCallBack(void)
{
//	INT32 copy_len = 0;
//	INT32 dst_len = 0;
    INT32 rec_date_len = 0;
	INT32 ret = 0;
	UINT8 *pRecv = NULL;

	X_DEBUG((">en5:"));
	rec_date_len = Xmodem_RecvOnce(&xcb, 1, &pRecv, 100);
	if(rec_date_len < 0)
	{
		ret = -1;
		goto done;
	}
	else if(rec_date_len == 0)
	{
		if(xcb.last_recv_cmd == XMODEM_CMD_EOT)
		{
			ret = xcb_recv_len;
			X_DEBUG(("EOT,len=%d", xcb_recv_len));
			goto done;
		}
		else
		{
			goto done;
		}
	}
#if 0
	/* xcb_recv_len_once > 0, copy data */
	dst_len = XCB_RECV_BUF_SIZE - xcb_recv_len_once%XCB_RECV_BUF_SIZE;
	copy_len = xcb_recv_len_once > dst_len ? dst_len: xcb_recv_len_once;
	
	if(copy_len > 0)
	{
		memcpy(xcb_buf+xcb_recv_len%XCB_RECV_BUF_SIZE, pRecv, copy_len);
		xcb_recv_len += copy_len;
	}
#else
    if(rec_date_len > 0)
    {
        memcpy(xcb_buf, pRecv, rec_date_len);
        xcb_recv_len = rec_date_len;
    }
#endif
	X_DEBUG(("ex5\r\n"));
done:
	return ret;
}

UINT8 *Xmodem_GetCallbackData(UINT32 *len)
{
	*len = xcb_recv_len;
	return xcb_buf;
}

INT32 Xmodem_RecvToFlash(xmodem_t *x, INT32 dev, UINT32 addr, INT32 dst_len, INT32 timeout)
{
	INT32 recv_len_total = 0;
	INT32 recv_len_once = 0;
	INT32 copy_len = 0;
	UINT8 *pRecv = NULL;
	writeFlashFlg = true;
	X_DEBUG((">en6:"));
	while((dst_len > 0) || (x->last_recv_cmd != XMODEM_CMD_EOT))
	{
		if(Device_Select(dev, timeout) <= 0)
		{
			X_DEBUG(("timeout!"));
			break;
		}

        if (true == Device_Recv_pend(EVENT_WAIT_US(1000000))){
            recv_len_once = Xmodem_RecvOnce(x, dev, &pRecv, timeout);
        }else {
            recv_len_once = -1;
        }

		if(recv_len_once < 0)
		{
			X_DEBUG(("len = %d, < 0!", recv_len_once));
			break;
		}
		
		copy_len = recv_len_once > dst_len ? dst_len: recv_len_once;
		if(copy_len == 0)
		{
			if(x->last_recv_cmd == XMODEM_CMD_EOT)
			{
				X_DEBUG(("recv EOT len=%d.", recv_len_total));
				break;
			}
			else
			{
				continue;
			}
		}

		BSP_GPIO_ToggleDebugPin();
		if(Flash_Write(addr, pRecv, copy_len) == FALSE)
		{
			X_DEBUG(("flash write error"));
			break;
		}
		BSP_GPIO_ToggleDebugPin();

		addr += copy_len;
		dst_len -= copy_len;
		recv_len_total += copy_len;
		
		copy_len = 0;
	}
	writeFlashFlg = false;
	X_DEBUG(("ex6\r\n"));
	return recv_len_total;
}

INT32 Xmodem_SendFromFlash(xmodem_t *x, INT32 dev, UINT32 addr, INT32 len, INT32 timeout)
{
	UINT32 paddr = addr;
	INT32 left_len = len;
	INT32 send_len_once = 0;
	INT32 send_len_total = 0;
	UINT8 send_buf[XMODEM_LEN_DAT] = {0};
	INT32 copy_len = 0;

	X_DEBUG((">en7:"));
	/* init send */
	x->send_sn = 1;
	/* send data */
	while(left_len > 0)
	{	
		copy_len = left_len > XMODEM_LEN_DAT ? XMODEM_LEN_DAT : left_len;	
		
		if(Flash_Read(paddr, send_buf, copy_len) == FALSE)
		{
			X_DEBUG(("flash read error!"));
			break;
		}
		
		send_len_once = Xmodem_SendOnce(x, dev, send_buf, copy_len, timeout);
		if(send_len_once > 0) //ack
		{
			paddr += send_len_once;
			left_len -= send_len_once;
			send_len_total += send_len_once;
			send_len_once = 0;
		}
		else if(send_len_once == 0) // nak or other
		{
			continue;
		}
		else // "NAK" or other, retry
		{
			send_len_total = 0;
			break;
		}
	}
	
	/* all data has been sent, send eot */
	if(left_len <= 0)
	{
		if(Xmodem_SendCmd(dev, XMODEM_CMD_EOT, 1, timeout) != 1)
		{
			send_len_total = 0;
		}
	}
	X_DEBUG(("ex7\r\n"));

	return send_len_total;
}
extern volatile UINT32 core_idel_flag;
void readCallback(UART_Handle handle, void *rxBuf, size_t size)
{
    if ((recv_once_buf[2] | (uint16_t)recv_once_buf[3]<<8)==CORE_CMD_BACK_TO_IDLE &&
        recv_once_buf[0]==XMODEM_CMD_SOH && XMODEM_LEN_ALL==size){
        core_idel_flag = 1;
    }else if (recCmdAckFlg == true && XMODEM_LEN_CMD==size){
        Device_Recv_post();
    }else if((XMODEM_LEN_CMD==size || XMODEM_LEN_ALL==size) && writeFlashFlg == true){
        Device_Recv_post();
    }else if (XMODEM_LEN_CMD==size || XMODEM_LEN_ALL==size){
        Event_communicateSet(EVENT_COMMUNICATE_RX_HANDLE);
    }else{
        Xmodem_InitCallback();
    }
    xcb_recv_len_once = size;
    UART_appRead(recv_once_buf, XMODEM_LEN_ALL);
}
