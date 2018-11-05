#include "rcuplink.h"
#include "cc2640r2_rf.h"
#include "timer.h"
#include "bsp.h"
#include "debug.h"
#include <string.h>
#include "crc16.h"
#include <stdio.h>
#include "core.h"



								
#define OFFSET_OF_REQDATA		6 //cmd+cmd_len, UINT16+INT32

#define STATE_OF_INIT			0
#define STATE_OF_RECV			1
#define STATE_OF_CHECK_RECV		2
#define STATE_OF_HANDLE			3
#define STATE_OF_UPLINK			4
#define STATE_OF_ACKRC			5
#define STATE_OF_EXIT			99

static UINT8 rcreq_timeout_timer = TIMER_UNKNOW;

static UINT8 rcreq_set_timer(UINT32 timeout)
{
	if((timeout != 0) && (rcreq_timeout_timer == TIMER_UNKNOW))
	{	
		if(timeout > 60000)
		{
			timeout = 60000;
			pinfo("warning: rcreq timeout change to 60000.\r\n");
		}
		
		if((rcreq_timeout_timer=TIM_Open(1000, timeout, TIMER_UP_CNT, TIMER_ONCE)) == TIMER_UNKNOW)
		{
			perr("open rcreq_timeout_timer, timer: %d, timeout: %d\r\n", 
				rcreq_timeout_timer, timeout);
		}
	}
	
	return rcreq_timeout_timer;
}

static UINT8 rcreq_check_timeout(void)
{
	if(rcreq_timeout_timer == TIMER_UNKNOW)
	{
		return 0;
	}
	else
	{
		return TIM_CheckTimeout(rcreq_timeout_timer);
	}
}

static void rcreq_close_timer(void)
{
	if(rcreq_timeout_timer != TIMER_UNKNOW)
	{
		TIM_Close(rcreq_timeout_timer);
		rcreq_timeout_timer = TIMER_UNKNOW;
	}
}

static INT32 rcreq_check_crc(UINT8 *src, UINT8 len)
{
	UINT16 read_crc=0, cal_crc=0;
	
	memcpy(&read_crc, src+len-2, sizeof(read_crc));

	cal_crc = CRC16_CaculateStepByStep(cal_crc, src, len-2);

	if(cal_crc == read_crc)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

static INT32 rcreq_check_data(UINT8 *src, UINT8 len)
{
	if(rcreq_check_crc(src, len) < 0)
	{
		perr("rcreq_check_crc\r\n");
		return -1;
	}

	if(!IS_VALID_CTRL(src[0]))
	{
		perr("ivalid ctrl: %02X\r\n", src[0]);
		return -2;
	}

	return 0;
}

extern UINT32 core_idel_flag;

static INT16 _init_recv(rcreq_table_t *table)
{
	pdebug("init recv\r\n");
	pdebughex((UINT8 *)table, 23);

	/* reset recv buf */
	table->data_num = 0;

	enter_txrx();
	
	//set recv para
	//enable rx
//	set_rx_start();

	//start timer
	rcreq_set_timer(table->timeout);

	return set_rx_para(table->id, table->recv_bps, table->channel, table->recv_len, table->timeout);
}

static INT32 _recv(rcreq_table_t *table)
{
	INT32 ret = 0;
	
	pdebug("_recv\r\n");
	while(1)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pinfo("back to idel\r\n");
			core_idel_flag = 0;
			ret = -1;
			break;
		}
		
		if(rcreq_check_timeout() == 1)
		{
			pinfo("timeout\r\n");
			ret = 0;
			break;
		}

		if(check_rx_status(1) == 0)
		{
			ret = 1;
			break;
		}
	}
	return ret;
}

static INT32 _stop_recv(void)
{
	rcreq_close_timer();
	exit_txrx();
	return 0;
}

static INT32 _check_recv(rcreq_table_t *table)
{
	UINT8 *ptr = table->data_buf+OFFSET_OF_REQDATA;
	
	if(get_rx_data(ptr, table->recv_len) == 0)
	{
		perr("rf hardware crc\r\n");
		return 0;
	}
	
	if(rcreq_check_data(ptr, table->recv_len) == 0)
	{			
		table->data_num += 1;
		*(ptr+table->recv_len) = get_recPkgRSSI();
		return 1;
	}
	else
	{
		return -1;
	}
}

static INT32 _streq(rcreq_table_t *table)
{
	INT16 apid = 0;

	memcpy(&apid, table->data_buf+OFFSET_OF_REQDATA+5, sizeof(apid));
	pinfo("_streq, apid=%d, pattern=%d\r\n", apid, table->recv_pattern);	
	if(apid==table->recv_pattern)
	{
		return 1;
	}
	else
	{
		return 0;
	}	
}

static UINT8 _get_req_ctrl(rcreq_table_t *table)
{
	return *(table->data_buf+OFFSET_OF_REQDATA);
}

static void _get_rc_id(rcreq_table_t *table, UINT8 *id, UINT8 idsize)
{
	memcpy(id, table->data_buf+OFFSET_OF_REQDATA+1, idsize);
}

static INT32 _uplink(rcreq_table_t *table)
{
	UINT16 cmd = CORE_CMD_ACK;
	INT32 cmd_len = table->data_num*(table->recv_len+1);
	
	memcpy(table->data_buf, &cmd, sizeof(cmd));
	memcpy(table->data_buf+sizeof(cmd), &cmd_len, sizeof(cmd_len));
	table->data_len = sizeof(cmd) + sizeof(cmd_len) + cmd_len;

	table->uplink_ret = table->uplink(table->data_buf, table->data_len);
	
	pinfo("rc uplink: %d\r\n", table->uplink_ret);
	phex(table->data_buf, table->data_len);
	
	return table->uplink_ret;
}

static void _make_rc_ack(rcreq_table_t *table)
{
	UINT16 crc = 0;
	UINT8 *buf = table->rc_ack_buf;
	
	buf[0] = _get_req_ctrl(table)+1;
	_get_rc_id(table, &buf[1], 4);
	memcpy(&buf[5], (UINT8*)&table->recv_pattern, sizeof(table->recv_pattern));
	buf[7] = table->uplink_ret;
	table->rc_ack_len = table->recv_len;
	crc = CRC16_CaculateStepByStep(crc, buf, table->rc_ack_len-sizeof(crc));
	memcpy(buf+table->rc_ack_len-sizeof(crc), &crc, sizeof(crc));
}

static INT32 _ack_rc(rcreq_table_t *table)
{
	UINT8 rcid[4] = {0};
	
	pdebug("ack rc\r\n");
	
	_make_rc_ack(table);
	_get_rc_id(table, rcid, sizeof(rcid));
	set_power_rate(table->ack_power, table->ack_bps);
    set_frequence(table->channel);
    send_data(rcid, table->rc_ack_buf, table->rc_ack_len, 2000);
	
	return 0;
}

INT32 RcReq_Mainloop(rcreq_table_t *table, UINT8 (*uplink)(UINT8 *src, UINT32 len))
{
	INT32 state = STATE_OF_INIT;
	INT32 ret = 0;
	int16_t rec_handle;
	
	while(1)
	{
		switch(state)
		{
			case STATE_OF_INIT:
			    rec_handle = _init_recv(table);
				table->uplink = uplink;
				state = STATE_OF_RECV;
				break;
			case STATE_OF_RECV:
				ret = _recv(table);
				if(ret == 1)
				{			
					state = STATE_OF_CHECK_RECV;
				}
				else if(ret == 0)
				{
					_uplink(table);
					state = STATE_OF_EXIT;
				}
				else
				{
					state = STATE_OF_EXIT;
				}
				RF_cancle(rec_handle);
				break;
			case STATE_OF_CHECK_RECV:
				ret = _check_recv(table);
				//pinfo("check recv return: %d\r\n", ret);
				if(ret <= 0) // -1, 0
				{
					state = STATE_OF_INIT;
				}
				else // 1
				{
					state = STATE_OF_HANDLE;
				}
				break;
			case STATE_OF_HANDLE:
				if(_get_req_ctrl(table) == CTRL_OF_RCREQ)
				{
					BSP_Delay1MS(table->recv_pattern);
					state = STATE_OF_ACKRC;
				}
				else if(_get_req_ctrl(table) == CTRL_OF_STREQ)
				{
					if(_streq(table) == 1)
					{
						state = STATE_OF_ACKRC;
					}
					else
					{
						state = STATE_OF_INIT;
					}
				}
				else
				{
					state = STATE_OF_INIT;
				}
				break;
			case STATE_OF_ACKRC:
				BSP_Delay1MS(2);
				_ack_rc(table);
				state = STATE_OF_UPLINK;
				break;				
			case STATE_OF_UPLINK:
				_uplink(table);
				state = STATE_OF_EXIT;
				break;
			default:
				perr("rcreq unknown status: %d\r\n", state);
				break;
		}

		if(state == STATE_OF_EXIT)
		{
			_stop_recv();
			pinfo("exit rcuplink\r\n");
			break;
		}
	}

	return ret;
}

INT32 RcReq_ParseCmd(UINT8 *pCmd, INT32 cmdLen, rcreq_table_t *table)
{
	memcpy(table->id, pCmd, cmdLen);

	return 0;
}

