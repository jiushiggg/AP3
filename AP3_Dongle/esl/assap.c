#include <cc2640r2_rf.h>
#include "assap.h"
#include <string.h>
#include "debug.h"
#include "common.h"
#include "bsp.h"
#include "core.h"

#define TIMEOUT_OF_RF_RX			20000 //20ms
#define MAX_COUNT_OF_RX_WKUP		3
#define CTRL_OF_GROUP_WKUP			0xA0
#define CTRL_OF_SET_WKUP1			0x60
#define CTRL_OF_SET_WKUP2			0x40
#define MASK_OF_WKUP_CTRL			0xE0
#define STATUS_OF_NONE_WKUP			102

#define LEN_OF_ESLID				4
#define OFFSET_OF_ESLID_IN_ACKBUF	20
#define OFFSET2_OF_ESLID_IN_ACKBUF	19
#define OFFSET_OF_ACK_IN_ACKBUF		4
#define VALUE_OF_ACK				0x40

#pragma pack(1)
typedef struct{
	UINT16 datarate;
	UINT8 power;
	UINT8 duration;
	UINT8 slot;
	UINT16 frame_num;
	UINT8 reserved[8];
	UINT8 wkupid[4];
	UINT8 wkupch;
	UINT8 wkupdatalen;
	UINT8 wkupdata[26];
}assack_cmd_para_t;

typedef struct {
	char timestamp[16];
	UINT16 num;
	UINT8 type;
	int timeout;
	UINT8 reserved[8];
}assack_cmd_head_t;

typedef struct {
	assack_cmd_head_t assack_cmd_head;
	assack_cmd_para_t assack_cmd_para[10];
}assack_cmd_t;

typedef struct {
	UINT16 cmd;
	INT32 cmd_len;
	UINT8 status;
	INT16 num;
}assap_scanwkup_ret_t;
#pragma pack()

assack_cmd_t assack_cmd;
extern UINT32 core_idel_flag;

INT32 assap_scanwkup_parse_cmd(UINT8 *cmd, INT32 cmd_len)
{
	if(cmd_len > sizeof(assack_cmd))
	{
		return -1;
	}
	else
	{
		memcpy(&assack_cmd, cmd, cmd_len);
		return 0;	
	}
}

INT32 assap_scanwkup_ret_size(void)
{
	return sizeof(assap_scanwkup_ret_t);
}

static INT32 _check_wkup_buf(UINT8 *wkupbuf, UINT8 len, UINT8 type)
{	
	if(type == 2)
	{
		if((wkupbuf[0]&MASK_OF_WKUP_CTRL) == CTRL_OF_GROUP_WKUP)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if(((wkupbuf[0]&MASK_OF_WKUP_CTRL)==CTRL_OF_SET_WKUP1) 
			|| ((wkupbuf[0]&MASK_OF_WKUP_CTRL)==CTRL_OF_SET_WKUP2))
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
}

static INT16 _get_wkup_rest_seconds(UINT8 *wkupbuf, UINT8 slot)
{
//	INT32 cnt = 0;
//	cnt = wkupbuf[0]&0x1F;
//	cnt = cnt << 8;
//	cnt |= wkupbuf[1];
//	cnt = cnt * slot / 1000;
//	return (INT16)cnt;
	return 1;
}

INT32 assap_scan_wkup(UINT8 *dst, INT32 dsize)
{
	assap_scanwkup_ret_t *p_assap_scanwkup_ret = (assap_scanwkup_ret_t *)dst;
	assack_cmd_t *p_assack_cmd = &assack_cmd;
	UINT8 wkupbuf[26] = {0};
	UINT8 t = 0;
	INT32 count = 0;
	
//	phex((UINT8 *)p_assack_cmd, sizeof(assack_cmd_t));
	
	set_power_rate(RF_DEFAULT_POWER, p_assack_cmd->assack_cmd_para[0].datarate);
	set_frequence(p_assack_cmd->assack_cmd_para[0].wkupch);
	t = set_timer(p_assack_cmd->assack_cmd_head.timeout);
	enter_txrx();
	
	while(1)
	{
		if(core_idel_flag == 1)
		{
			pinfo("back to idel\r\n");
			core_idel_flag = 0;
			return 0;
		}
	
		if(check_timer_timeout(t) == 1)
		{
			pinfo("timeout\r\n");
			break;
		}
		
		if( recv_data(p_assack_cmd->assack_cmd_para[0].wkupid, wkupbuf, 
						p_assack_cmd->assack_cmd_para[0].wkupdatalen, 
						TIMEOUT_OF_RF_RX) \
						== p_assack_cmd->assack_cmd_para[0].wkupdatalen)
		{
			if(_check_wkup_buf(wkupbuf, p_assack_cmd->assack_cmd_para[0].wkupdatalen, p_assack_cmd->assack_cmd_head.type) == 0)
			{
				count++;
				if(count >= MAX_COUNT_OF_RX_WKUP)
				{
					break;
				}
			}
		}
	}

	close_timer(t);
	exit_txrx();
	
	pinfo("scaned wkup %d count(s)\r\n", count);

	p_assap_scanwkup_ret->cmd = CORE_CMD_ACK;
	p_assap_scanwkup_ret->cmd_len = sizeof(UINT8)+sizeof(INT16);
	if(count >= MAX_COUNT_OF_RX_WKUP)
	{
		p_assap_scanwkup_ret->status = 0;
		p_assap_scanwkup_ret->num = _get_wkup_rest_seconds(wkupbuf, p_assack_cmd->assack_cmd_para[0].slot);
	}
	else
	{
		p_assap_scanwkup_ret->status = STATUS_OF_NONE_WKUP;
		p_assap_scanwkup_ret->num = 0;
	}
		
	return sizeof(assap_scanwkup_ret_t);
}



INT32 assap_ack_parse_cmd(UINT8 *pCmd, INT32 cmdLen, assap_ack_table_t *table)
{
	return common_recv_parse_cmd(pCmd, cmdLen, table);
}

static INT32 _check_ack(UINT8 *ptr, UINT8 len, UINT8 *id)
{
	UINT8 eslid[4] = {0};
	UINT16 crc1, crc2;
	
	if((ptr[0]&0xE0) != (3 << 5)) //check ctrl
	{
		return -2;
	}
	
	if(ptr[OFFSET_OF_ACK_IN_ACKBUF] != VALUE_OF_ACK)
	{
		return -1;
	}
	
	memcpy(eslid, ptr+OFFSET_OF_ESLID_IN_ACKBUF, LEN_OF_ESLID);
	crc1 = ptr[24]+ptr[25]*256;
	crc2 = cal_crc16(ptr[0], eslid, ptr+1, 23);
	if(crc1 != crc2)
	{
		memcpy(eslid, ptr+OFFSET2_OF_ESLID_IN_ACKBUF, LEN_OF_ESLID);
		crc2 = cal_crc16(ptr[0], eslid, ptr+1, 23);
		if(crc1 != crc2)
		{
			return -3;
		}
	}
	
	memcpy(id, eslid, LEN_OF_ESLID);
	return 0;
}

INT32 assap_ack(assap_ack_table_t *table)
{
	UINT8 *ptr = NULL;
	INT32 recv_len_total = 0;
	UINT8 t = 0;
	
//	pinfo("assap_ack(), id=0x%02X-0x%02X-0x%02X-0x%02X, ch=%d, recv_datarate=%d, recv_len=%d, interval=%d, timeout=%d, lenout=%d, numout=%d, loop=%d\r\n", \
//			table->id[0], table->id[1], table->id[2], table->id[3], table->channel, table->recv_bps, \
//			table->recv_len, table->interval, table->timeout, table->lenout, table->numout, table->loop);
	
	/* reset recv buf */
	table->num = 0;
	table->data_len = 8;
	ptr = table->data_buf+8;

	t = set_timer(table->timeout);
	set_power_rate(RF_DEFAULT_POWER, table->recv_bps);
	set_frequence(table->channel);
	enter_txrx();
	
	while(1)
	{
		if(core_idel_flag == 1)
		{
			pinfo("back to idel\r\n");
			core_idel_flag = 0;
			recv_len_total = -1;
			break;
		}
		
		if((table->data_len+table->recv_len) > G3_HB_BUF_SIZE)
		{
			pinfo("memout\r\n");
			break;
		}
		
		if(((table->data_len+table->recv_len)>table->lenout) && (table->lenout!=0))
		{
			pinfo("lenout\r\n");
			break;
		}
		
		if(check_timer_timeout(t) == 1)
		{
			//pinfo("timeout\r\n");
			break;
		}
			
		if((table->num>=table->numout) && (table->numout!=0))
		{
			pinfo("numout\r\n");
			break;
		}
		
		if(table->num>=60000)
		{
			pinfo("numout 60000\r\n");
			break;
		}
		
		if(recv_data(table->id, ptr+LEN_OF_ESLID, table->recv_len, 20000) == table->recv_len) //20ms
		{
//			pinfo("recved: ");
//			phex(ptr+LEN_OF_ESLID, table->recv_len);
			if(_check_ack(ptr+LEN_OF_ESLID, table->recv_len, ptr) == 0)
			{
				table->num += 1;
				pdebug("assap_ack: %d:", table->num);	
				pdebughex(ptr, table->recv_len+LEN_OF_ESLID);
				ptr += table->recv_len+LEN_OF_ESLID;
				table->data_len += table->recv_len+LEN_OF_ESLID;
				recv_len_total += table->recv_len+LEN_OF_ESLID;
			}
		}

		BSP_Delay1MS(table->interval);
	}
	
	close_timer(t);
	
	exit_txrx();
	
	if(recv_len_total >= 0)
	{
		UINT16 cmd = CORE_CMD_ACK;
		UINT32 cmd_len = recv_len_total + 2; //data len + sizeof(num)
		memcpy(table->data_buf, (UINT8 *)&cmd, 2);//set cmd
		memcpy(table->data_buf+2, (UINT8 *)&cmd_len, 4);//set len
		memcpy(table->data_buf+6, (UINT8 *)&table->num, 2);//set len
	}
	
	pinfo("assap_ack:%d,%d\r\n", table->num, recv_len_total);
	
	return recv_len_total;
}

