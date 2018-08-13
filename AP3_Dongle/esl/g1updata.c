#include "g1updata.h"
#include "timer.h"
#include "bsp.h"
#include "flash.h"
#include "debug.h"
#include "cc2640r2_rf.h"
#include <string.h>
#include "data.h"
#include "updata0.h"
#include "frame1.h"
#include "sleep.h"
#include "core.h"

#define RF_ACK_TIMEOUT	     	0X00
#define RF_ACK_OK           	0x40
#define RF_ACK_RECVERR			0X01
#define RF_ACK_LOWBATT         	0X02
#define RF_ACK_CRCERR			0XFF
#define RF_ACK_CRCERR1			0XEE
#define RF_ACK_IDERR			0xFE
#define RF_ACK_DATAERR			0xFD

#pragma pack(1)
typedef struct {
	UINT32 addr;
	UINT8 ack;
} esl_t;
#pragma pack(1)

#pragma pack(1)
typedef struct {
	UINT8 myid[4];
	UINT16 tx_datarate;
	UINT16 rx_datarate;
	UINT8 deal_duration;
} transfer_para_t;
#pragma pack(1)

#define RF_MAX_BUF_SIZE			64

#pragma pack(1)
typedef struct {
	UINT8 id[4];
	UINT8 channel;
	UINT8 len;
	UINT8 data[RF_MAX_BUF_SIZE];
} transfer_data_t;
#pragma pack(1)

INT32 g1_init_data(UINT32 addr, INT32 len, updata_table_t *table)
{
	esl_t *pESL = (esl_t *)&table->data[0];	
	INT32 left_pkg_num = TABLE_BUF_SIZE / sizeof(esl_t);

	INT32 left_len = len;
	UINT32 cur = addr;
	INT32 offset = 0;

	pdebug("g1 init data\r\n");

	while((left_len > 0) && (left_pkg_num > 0))
	{
		offset = get_one_data(cur, NULL, NULL, NULL, NULL, 0);
		
		pdebug("get one, addr: 0x%08X, left_len: %d\r\n", cur, left_len);
		
		if(offset <= 0)
		{
			perr("g1 init data get data\r\n");
			break;
		}

		pESL->addr = cur;
		pESL->ack = 0;

		table->real_pkg_num++;

		left_pkg_num--;
		pESL++;
		cur += offset;
		left_len -= offset;
	}

	if(table->real_pkg_num != table->pkg_num)
	{
		pinfo("warning: recv pkg num too big(%d,%d), auto adjust\r\n", table->real_pkg_num, table->pkg_num);
	}
	table->ok_esl_num = 0;

	return table->real_pkg_num;
}

static UINT8 g1_general_data_transfer(transfer_data_t* pData, transfer_para_t* pPara, UINT8 sort)
{
	UINT8 ret = 0;
	UINT8 ack_buf[6] = {0};
	INT32 to = pPara->deal_duration * 10; // unit is 100us
	

	set_power_rate(RF_DEFAULT_POWER, pPara->tx_datarate);
	set_frequence(pData->channel);
	pData->data[1] = sort;
	send_data(pData->id, pData->data, pData->len, 2000);

	set_power_rate(RF_DEFAULT_POWER, pPara->rx_datarate);
	set_frequence(pData->channel);
    if(recv_data(pPara->myid, ack_buf, sizeof(ack_buf), 8000) == 0)
    {
        pdebug("recv timeout.\r\n");
        goto done;
    }
	
	if((ack_buf[2] == pData->id[0])
		&& (ack_buf[3] == pData->id[1])
		&& (ack_buf[4] == pData->id[2])
		&& (ack_buf[5] == pData->id[3]))
	{		
		ret = ack_buf[1];
	}
	else
	{
		ret = RF_ACK_IDERR;
	}
	
done:
	while((to--) > 0)
	{
		BSP_Delay100US(1);
	}
	
	return ret;
}

#define ESL_ACK_TIMEOUT			0X0
#define ESL_ACK_OK				0X1
#define ESL_ACK_LOWPOWER		0X2
#define ESL_ACK_CRCERR			0X3
#define ESL_ACK_OTHERERR		0X4
#define ESL_ACK_ASSBSERR1		0XF
#define ESL_ACK_ASSBSERR2		0XE
#define ESL_ACK_ASSBSERR3		0XD

UINT8 convert_ack(UINT8 old_ack)
{
	UINT8 new_ack = 0;

	switch(old_ack)
	{
		case 0:
			new_ack = ESL_ACK_TIMEOUT;
			break;
		case 0x02:
			new_ack = ESL_ACK_LOWPOWER;
			break;
		case 0x40:
			new_ack = 0x40;
			break;
//		case 0xFF:
//			new_ack = ESL_ACK_CRCERR;
//			break;
//		case 0xEE:
//			new_ack = ESL_ACK_ASSBSERR1;
//			break;			
//		case 0xFE:
//			new_ack = ESL_ACK_ASSBSERR2;
//			break;
//		case 0XFD:
//			new_ack = ESL_ACK_ASSBSERR3;
//			break;
		default:
//			new_ack = ESL_ACK_OTHERERR;
			break;
	}

	return new_ack;
}

UINT8 g1_updata_one(transfer_data_t* pData, transfer_para_t* pPara, UINT8 mode)
{
	UINT8 ack = 0;

	mode = mode % 2;
	
	if(mode == 0)
	{
		ack = g1_general_data_transfer(pData, pPara, 1);
		ack = convert_ack(ack);
	}
	else
	{
		INT32 i = 0;
		INT32 ok_times = 0;
		for(i = 0; i < 4; i++)
		{
			ack = g1_general_data_transfer(pData, pPara, (i+1)|0xF0);
			if(ack == 0x40)
			{
				ok_times++;
			}
		}

		if(ok_times >= 3)
		{
			ack = 0x40;
		}
		else
		{
			ack = 0;
		}
	}

	return ack;
}

void g1_sleep_one(UINT8 *id, UINT8 ch, UINT8 id_ctrl_x, UINT8 len)
{
	UINT8 buf[64] = {0};

	buf[0] = 0xe0 | (id[id_ctrl_x] & 0x1f);
	set_frequence(ch);
	send_data(id, buf, len, 2000);
}

void convert_transfer_para(transfer_para_t *para, updata_table_t *table)
{
	memcpy(para->myid, table->master_id, 4);
	para->tx_datarate = table->tx_datarate;
	para->rx_datarate = table->rx_datarate;
	para->deal_duration = table->deal_duration;
}

static transfer_data_t td;
static transfer_para_t tp;

INT32 g1_updata_loop(updata_table_t *table)
{
	UINT8 t = 0;
	INT32 ret = 0;
	esl_t *pESL = (esl_t *)&table->data[0];
	INT32 i = 0;

	pdebug("g1 updata loop\r\n");
	
	if((t=TIM_Open(100, table->esl_work_duration*10, 0, TIMER_ONCE)) == TIMER_UNKNOW)
	{
		ret = -1;
		goto done;
	}

	convert_transfer_para(&tp, table);
	
	while(!TIM_CheckTimeout(t))
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("g1_updata_loop Core_GetQuitStatus()=1\r\n");
			break;		
		}
		
		if(table->ok_esl_num == table->real_pkg_num)
		{
			pdebug("all pkg tx ok, exit\r\n");
			break;
		}

		if(pESL[i].ack != 0)
		{
			goto user_continue;
		}

		get_one_data(pESL[i].addr, td.id, &td.channel, &td.len, td.data, RF_MAX_BUF_SIZE);
		pESL[i].ack = g1_updata_one(&td, &tp, table->mode);
		pdebug("%d/%d, 0x%02X-0x%02X-0x%02X-0x%02X: 0x%02x", i+1, table->real_pkg_num,
									td.id[0], td.id[1], td.id[2], td.id[3], pESL[i].ack);
		if(pESL[i].ack != 0)
		{
		    set_power_rate(RF_DEFAULT_POWER, table->tx_datarate);
			pdebug(", sleep it\r\n", td.id[0], td.id[1], td.id[2], td.id[3]);
			g1_sleep_one(td.id, td.channel, table->id_x_ctrl, td.len);
			table->ok_esl_num++;
		}
		else
		{
			pdebug("\r\n");
		}
		
		/* 2rd generation, need dummy frame1 */
		if(table->mode >= 2)
		{
			if((((table->mode%2)==0)&&((i%8)==0)&&(i!=0)) || (((table->mode%2)==1)&&((i%2)==0)&&(i!=0)))
			{
				dummy(table, table->tx_duration);
				wait(1000);
			}
		}

user_continue:
		i++;
		if(i == table->real_pkg_num)
		{
			i = 0;
		}
	}

	TIM_Close(t);

	set_power_rate(RF_DEFAULT_POWER, table->tx_datarate);
	pESL = (esl_t *)&table->data[0];
	for(i = 0; i < table->real_pkg_num; i++)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("g1_updata_loop sleep quit\r\n");
			break;		
		}
		
		get_one_data(pESL[i].addr, td.id, &td.channel, &td.len, NULL, 0);
		pdebug("g1 sleep all: %02X-%02X-%02X-%02X, %d\r\n", 
			td.id[0], td.id[1], td.id[2], td.id[3], td.channel);
		g1_sleep_one(td.id, td.channel, table->id_x_ctrl, td.len);
	}

done:
	return ret;
}

INT32 g1_make_ack(updata_table_t *table, UINT32 *ack_addr, UINT32 *ack_len)
{
	UINT32 offset = 0;
	INT32 i;
	UINT8 id[4] = {0};
	esl_t *pESL = (esl_t *)&table->data[0];	
	
	pdebug("g1_make_ack()\r\n");
	
	*ack_len = table->real_pkg_num*5 + 4 + 19;
	*ack_addr = Flash_Malloc(*ack_len); 
	if(*ack_addr == MALLOC_FAIL)
	{
		perr("g1_make_ack malloc flash: addr = 0x%08X, len = %d!\r\n", \
				*ack_addr, *ack_len);
		*ack_len = 0;
		goto done;
	}
	else
	{
		pdebug("malloc flash: addr = 0x%08X, len = %d, success.\r\n", \
				*ack_addr, *ack_len);
	}
	
	offset = *ack_addr;
	if(g3_set_ack_para(offset, table->sid, 0x0900, table->real_pkg_num*5+11, 0, table->real_pkg_num) == 0)
	{
		perr("g1_make_ack set ack para!\r\n");
		*ack_len = 0;
		goto done;
	}
	offset += 21;

	for(i = 0; i < table->real_pkg_num; i++)
	{	
		get_one_data(pESL[i].addr, id, NULL, NULL, NULL, 0);
	
		if(g3_set_ack(offset, id, pESL[i].ack) == 0)
		{
			perr("g1_make_ack set one ack!\r\n");
			*ack_len = 0;
			goto done;
		}
		else
		{
			offset += 5;
		}
	}
	
	if(g3_set_ack_crc(*ack_addr, *ack_len-2) == 0)
	{
		perr("g1_make_ack set crc!\r\n");
		*ack_len = 0;
	}
	
done:	
	return 0;
}


