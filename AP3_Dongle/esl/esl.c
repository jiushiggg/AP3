#include "esl.h"
#include "data.h"
#include "debug.h"
#include "wakeup.h"
#include "frame1.h"
#include "updata.h"
#include "g2updata.h"
#include "g1updata.h"
#include <string.h>
#include "sleep.h"
#include "cc2640r2_rf.h"
#include "core.h"

static UINT16 set_cmd = 0;
static UINT32 set_addr = 0;
static INT32 set_len = 0;
static INT32 set_loop_times = 0;

static UINT32 wkup_addr = 0;
static INT32 wkup_len = 0;

static UINT16 frame1_cmd = 0;
static UINT32 frame1_addr = 0;
static INT32 frame1_len = 0;

static UINT32 sleep_addr = 0;
static INT32 sleep_len = 0;

static UINT16 updata_cmd = 0;
static UINT32 updata_addr = 0;
static INT32 updata_len = 0;

void reset_local_cmd(void)
{
	set_cmd = 0;
	set_addr = 0;
	set_len = 0;

	wkup_addr = 0;
	wkup_len = 0;
	
	frame1_cmd = 0;
	frame1_addr = 0;
	frame1_len = 0;
	
	sleep_addr = 0;
	sleep_len = 0;
	
	updata_cmd = 0;
	updata_addr = 0;
	updata_len = 0;
}

void debug_local_cmd(void)
{
	pdebug("+++++++++++++++++++++++++++++++++++++++++++++\r\n");
	pdebug("set cmd = 0x%04X, addr = 0x%08X, len = %d\r\n", set_cmd, set_addr, set_len);
	pdebug("wkup addr = 0x%08X, len = %d\r\n", wkup_addr, wkup_len);
	pdebug("frame1 addr = 0x%08X, len = %d\r\n", frame1_addr, frame1_len);
	pdebug("sleep addr = 0x%08X, len = %d\r\n", sleep_addr, sleep_len);
	pdebug("updata cmd = 0x%04X, addr = 0x%08X, len = %d\r\n", updata_cmd, updata_addr, updata_len);
	pdebug("+++++++++++++++++++++++++++++++++++++++++++++\r\n");
}

INT32 parse_cmd_data(UINT32 cmd_data_addr, UINT32 cmd_data_len)
{	
	INT32 ret = 0;
	UINT32 addr = cmd_data_addr;
	INT32 left_data_len = cmd_data_len;
	UINT16 cmd = 0;
	UINT32 cmd_len = 0;
	
	set_loop_times = 1;//default is 1
	
	while(left_data_len > 0)
	{
		cmd = g3_get_cmd(addr, &cmd_len);
		pdebug("get cmd=0x%04X, cmd_len=%d, cmd_addr=0x%08X, left len=%d.\r\n",
				cmd, cmd_len, addr, left_data_len);
	
		switch(cmd)
		{
			case CMD_SET_WKUP_TRN:
			case CMD_SET_WKUP_GLB:
			case CMD_SET_WKUP_CH:
			case CMD_SET_WKUP_BDC:
			case CMD_SET_LED_FLASH:
				set_cmd = cmd;
				set_addr = addr+sizeof(cmd)+sizeof(cmd_len);
				set_len = cmd_len;
				addr += sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				left_data_len -= sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				ret += 1;
				if(cmd == CMD_SET_WKUP_BDC)
				{
					set_loop_times = wakeup_get_loop_times(set_addr);
				}
				break;
			case CMD_GROUPN_WKUP:
				wkup_addr = addr+sizeof(cmd)+sizeof(cmd_len);
				wkup_len = cmd_len;
				addr += sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				left_data_len -= sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				ret += 1;
				break;	
			case CMD_GROUP1_FRAME1:
			case CMD_GROUP1_FRAME2:
			case CMD_GROUPN_FRAME1:
				frame1_cmd = cmd;
				frame1_addr = addr+sizeof(cmd)+sizeof(cmd_len);
				frame1_len = cmd_len;
				addr += sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				left_data_len -= sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				ret += 1;
				break;			
			case CMD_GROUP1_SLEEP:
			case CMD_GROUPN_SLEEP:
				sleep_addr = addr+sizeof(cmd)+sizeof(cmd_len);
				sleep_len = cmd_len;
				addr += sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				left_data_len -= sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				ret += 1;
				break;			
			case CMD_GROUP1_DATA:
			case CMD_GROUPN_DATA:
			case CMD_GROUPN_DATA_G2:	
			case CMD_GROUPN_DATA_G1:
			case CMD_GROUP1_DATA_BDC:
			case CMD_GROUP1_DATA_NEWACK:
			case CMD_GROUPN_DATA_NEWACK:
				updata_cmd = cmd;
				updata_addr = addr+sizeof(cmd)+sizeof(cmd_len);
				updata_len = cmd_len;
				addr += sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				left_data_len -= sizeof(cmd)+sizeof(cmd_len)+cmd_len;
				ret += 1;
				break;
			case CMD_SET_WKUP:
			case CMD_GROUP1:
			case CMD_GROUPN:
                addr += sizeof(cmd)+sizeof(cmd_len);
                left_data_len -= sizeof(cmd)+sizeof(cmd_len);
                break;
			default:				
			    ret = -1;
			    left_data_len = 0;
				break;
		}

	}

	debug_local_cmd();
	
	return ret;
}

static updata_table_t *updata_table = NULL;

INT32 esl_updata(esl_updata_t *updata)
{
	INT32 ret = 0;
	UINT32 addr = updata->data_addr;
	INT32 left_data_len = updata->data_len;

	pdebug("enter esl updata!\r\n");

	g3_print_data(addr, left_data_len);

	updata->ack_addr = 0;
	updata->ack_len = 0;
	
	updata_table = (updata_table_t *)updata->buf;
	
	updata->sid = g3_get_sid(addr);
	updata_table->sid = updata->sid;
	addr += 4;
	left_data_len -= 4;

	memset(updata->buf, 0, UPDATA_BUF_SIZE);
	reset_local_cmd();

	if(parse_cmd_data(addr, left_data_len) <= 0)
	{
		ret = -1;
		goto _ret;
	}
	
	updata_init_data(updata_cmd, updata_table, frame1_addr, frame1_len, updata_addr, updata_len);

	enter_txrx();

	while((set_loop_times--) > 0)
	{
		//step 1: tx set cmd
		if(set_addr != 0)
		{
			if((set_cmd==CMD_SET_WKUP_TRN)||(set_cmd==CMD_SET_WKUP_BDC))
			{	
				pdebug("set wkup trn & bdc, loop:%d\r\n", set_loop_times);
				pinfoEsl("sw0 bg\r\n");
				wakeup_start(set_addr, set_len, 0);
			}
			else if((set_cmd==CMD_SET_WKUP_GLB) || (set_cmd==CMD_SET_WKUP_CH))
			{
			    pinfoEsl("sw1 bg\r\n");
				pdebug("set wkup glb & ch\r\n");
#ifdef FLASH_LED_TEST
				set_wakeup_led_flash(set_addr, updata_table->data, set_len);//debug
#else
				wakeup_start(set_addr, set_len, 1);
#endif

			}
			else if (set_cmd == CMD_SET_LED_FLASH)
			{
				set_wakeup_led_flash(set_addr, &frame1_addr, updata_table->data, set_len);
			}
			pinfoEsl("sw ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit1\r\n");
			break;
		}	

		//step 2: tx group cmd
		if(wkup_addr != 0)
		{
			pdebug("group wkup");
			pinfoEsl("gw bg\r\n");
			wakeup_start(wkup_addr, wkup_len, 0);
			pinfoEsl("gw ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit2\r\n");
			break;
		}
		
		//step 3: tx frame1
		if(frame1_addr != 0)
		{
			pdebug("frame1\r\n");
			pinfoEsl("f1 bg\r\n");
			frame1_start(frame1_cmd, frame1_addr, frame1_len);
			pinfoEsl("f1 ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit3\r\n");
			break;
		}
		//step 4: tx sleep
		if(sleep_addr != 0)
		{
			pdebug("sleep\r\n");
			pinfoEsl("s4 bg\r\n");
			sleep_start(sleep_addr, sleep_len);
			pinfoEsl("s4 ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit4\r\n");
			break;
		}

		//step 5: tx updata and dummy frame1
		if(updata_addr != 0)
		{
			pdebug("updata\r\n");
			pinfoEsl("ud5 bg\r\n");
			updata_do_updata(updata_cmd, updata_table);
			pinfoEsl("up5 ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit5\r\n");
			break;
		}
        //step 6: tx frame1
        if((frame1_addr!=0) && (frame1_cmd!=CMD_GROUP1_FRAME2))
        {
            pdebug("frame1\r\n");
            pinfo("f1.6 bg\r\n");
            frame1_start(frame1_cmd, frame1_addr, frame1_len);
            pinfo("f1.6 ed\r\n");
        }
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit6\r\n");
			break;
		}

		//step 7: tx sleep
		if(sleep_addr != 0)
		{
			pdebug("sleep\r\n");
			pinfo("s7 bg\r\n");
			sleep_start(sleep_addr, sleep_len);
			pinfo("s7 ed\r\n");
		}
		if(Core_GetQuitStatus() == 1)
		{
			reset_local_cmd();
			pdebug("esl_updata quit7\r\n");
			break;
		}
	}
	
	//step 8: make ack
	if(updata_addr != 0)
	{
		pdebug("make ack\r\n");
		pinfo("make ack\r\n");
		updata_make_ack(updata_cmd, updata_table, (UINT32*)&updata->ack_addr, (UINT32*)&updata->ack_len);
	}

	reset_local_cmd();

	if((updata->ack_addr!=0) && (updata->ack_len!=0))
	{
		g3_print_ack(updata->ack_addr, updata->ack_len);
	}

	exit_txrx();

_ret:
	
	pdebug("exit esl updata\r\n");
	
	return ret;
}


