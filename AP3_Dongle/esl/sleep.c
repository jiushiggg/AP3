#include "cc2640r2_rf.h"
#include "sleep.h"
#include "flash.h"
#include "debug.h"
#include "updata.h"
#include "data.h"
#include "g1updata.h"
#include "common.h"
#include "bsp.h"
#include "core.h"

#define OFFSET_SLEEP_DATA	17
#define SIZE_MAX_ESL_BUF	64

static UINT32 sleep_addr = 0;
static INT32 sleep_len = 0;

/* sleep paras */
static UINT16 sleep_datarate = 0;
static UINT8 sleep_power = 0;
static UINT8 sleep_mode = 0;
static UINT8 sleep_interval = 0;
static UINT8 sleep_idx = 0;
static UINT8 sleep_times = 0;
static UINT8 sleep_default_len = 0;
static UINT16 sleep_num = 0;

/* one sleep data */
static UINT8 sleep_id[4] = {0};
static UINT8 sleep_channel = 0;
static UINT8 sleep_data[SIZE_MAX_ESL_BUF] = {0};
static UINT8 sleep_data_len = 0;

static INT32 sleep_mode0()
{
	INT32 ret = 0;
	INT32 i, j;
	UINT32 cur = 0;
	INT32 read_len = 0;
	volatile INT8 prev_channel=RF_FREQUENCY_UNKNOW;

	for(j = 0; j < sleep_times; j++)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("sleep_mode0 quit2\r\n");
			break;
		}
		
		ret = 0;
		cur = sleep_addr+OFFSET_SLEEP_DATA;
		
		for( i = 0; ((i<sleep_num) && (cur<sleep_addr+sleep_len)); i++)
		{
			if(Core_GetQuitStatus() == 1)
			{
				pdebug("sleep_mode0 quit1\r\n");
				break;
			}
			
			read_len = get_one_data(cur, sleep_id, &sleep_channel, &sleep_data_len, sleep_data, sizeof(sleep_data));
			if(read_len <= 0)
			{
				ret = -1;
				perr("sleep mode0, g1 get data\r\n");
				break;
			}
			
			if(sleep_data_len == 0)
			{
				sleep_data_len = sleep_default_len;
				make_sleep_data(sleep_id, sleep_idx, sleep_data, sleep_data_len);
			}

	        if (sleep_channel != prev_channel){
	            set_frequence(sleep_channel);
	        }
	        prev_channel = sleep_channel;
	        send_data(sleep_id, sleep_data, sleep_data_len, 1000);
//			send_data(sleep_id, sleep_data, sleep_data_len, sleep_channel, 1000);
			
			pdebug("sleep %02X-%02X-%02X-%02X, channel=%d, datalen=%d: ", \
					sleep_id[0], sleep_id[1], sleep_id[2], sleep_id[3], sleep_channel, sleep_data_len);
			pdebughex(sleep_data, sleep_data_len);
			
			BSP_Delay1MS(sleep_interval);
			
			cur += read_len;
		}
	}

	//set_cmd_stby();	
	
	return ret;
}

INT32 sleep_init(UINT32 addr, INT32 len)
{
	if((addr == 0) || (len == 0))
	{
		return -1;
	}
	
	sleep_addr = addr;
	sleep_len = len;
	
	Flash_Read(addr, (UINT8 *)&sleep_datarate, 2);
	Flash_Read(addr+2, &sleep_power, 1);
	Flash_Read(addr+3, &sleep_mode, 1);
	Flash_Read(addr+4, &sleep_interval, 1);
	Flash_Read(addr+5, &sleep_idx, 1);
	Flash_Read(addr+6, &sleep_times, 1);
	Flash_Read(addr+7, &sleep_default_len, 1);
	if((sleep_default_len==0) || (sleep_default_len > SIZE_MAX_ESL_BUF))
	{
		sleep_default_len = 26;
	}
	Flash_Read(addr+15, (UINT8 *)&sleep_num, 2);
	
	return 0;
}

void sleep_exit(void)
{
	sleep_addr = 0;
	sleep_len = 0;
}

INT32 sleep_start(const UINT32 addr, const UINT32 len)
{
	INT32 ret = 0;

	if(sleep_init(addr, len) < 0)
	{
		ret = -1;
		goto done;
	}

	pdebug("sleep:datarate=%d,power=%d,mode=%d,interval=%d,idx=%d,times=%d,num=%d,default_len=%d.\r\n", \
			sleep_datarate, sleep_power, sleep_mode, sleep_interval, sleep_idx, sleep_times, sleep_num, sleep_default_len);
	
	if(sleep_num == 0)
	{
		goto done;
	}
	
	set_power_rate(sleep_power, sleep_datarate);
	
	switch(sleep_mode)
	{
		case 0: // 0 is default
		default:
			if(sleep_mode0() < 0)
			{
				ret = -2;
			}
		break;
	}
	
	sleep_exit();
done:
	return ret;
}



