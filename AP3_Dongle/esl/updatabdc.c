#include "cc2640r2_rf.h"
#include "updatabdc.h"
#include "frame1.h"
#include "timer.h"
#include "debug.h"
#include "data.h"
#include "common.h"
#include "core.h"

//mode 0: 11111222223333334444444......xxxxxxx
static void _bdc_transmit_mode0(updata_table_t *table, UINT8 timer)
{
	UINT8 id[4] = {0};
	UINT8 ch = 0;
	UINT8 len = 0;
	UINT8 data[64] = {0};
	INT32 i, j, k;
	UINT32 txaddr = table->updata_addr+OFFSET_DATA_DATA;
    RF_EventMask result;
    uint8_t pend_flg = PEND_STOP;

	i = j = 0;
	k = table->tx_interval*1000/table->tx_duration+1;
	k = 3;		//todo:
	while(1)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("_bdc_transmit_mode0 Core_GetQuitStatus()=1\r\n");
			break;		
		}
		
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			pdebug("_bdc_transmit_mode0 timeout.\r\n");
			break;
		}
		
		if(j == 0)
		{
			get_one_data(txaddr, id, &ch, &len, data, sizeof(data));
		}

        if (PEND_START == pend_flg){
            send_pend(result);
        }
        result = send_without_wait(id, data, len, ch, 6000);
        pend_flg = PEND_START;
		j++;
		if(j >= k)
		{
			txaddr += sizeof(id)+sizeof(ch)+sizeof(len)+len;
			j = 0;
			i++;
		}
		
		if(i >= table->num)
		{
			txaddr = table->updata_addr+OFFSET_DATA_DATA;
			i = 0;
		}
	}
	
	wait(2000);
}

//mode 1: 12345....x12345....x12345....x.......
static void _bdc_transmit_mode1(updata_table_t *table, UINT8 timer)
{
	UINT8 id[4] = {0};
	UINT8 ch = 0;
	UINT8 len = 0;
	UINT8 data[64] = {0};
	INT32 i = 0;
	UINT32 txaddr = table->updata_addr+OFFSET_DATA_DATA;
    RF_EventMask result;
    uint8_t pend_flg = PEND_STOP;

	while(1)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("_bdc_transmit_mode1 Core_GetQuitStatus()=1\r\n");
			break;		
		}
		
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			pdebug("_bdc_transmit_mode1(%d) timeout\r\n", timer);
			break;
		}
		
		if((i >= table->num) || (i == 0))
		{
			txaddr = table->updata_addr+OFFSET_DATA_DATA;
			i = 0;
		}
		
		get_one_data(txaddr, id, &ch, &len, data, sizeof(data));

        if (PEND_START == pend_flg){
            send_pend(result);
        }
        result = send_without_wait(id, data, len, ch, 6000);
        pend_flg = PEND_START;
		txaddr += sizeof(id)+sizeof(ch)+sizeof(len)+len;
		i++;
	}
	
	wait(2000);
}

UINT8 bdc_updata_loop(updata_table_t *table)
{
	UINT8 ret = 0;
	UINT8 timer = 0;
	UINT16 timeout = table->esl_work_duration * 10;
		
	if((timer=TIM_Open(100, timeout, TIMER_UP_CNT, TIMER_ONCE)) == TIMER_UNKNOW)
	{
		perr("bdc_updata_loop open timer\r\n");
		goto done;
	}
	pdebug("bdc_updata_loop, timer %d timeout is %d\r\n", timer, timeout);

	set_power_rate(table->tx_power, table->tx_datarate);

	if(table->mode == 1)
	{
		_bdc_transmit_mode1(table, timer);
	}
	else //0 and default mode
	{
		_bdc_transmit_mode0(table, timer);
	}
	
	TIM_Close(timer);
		
	ret = 1;
	
done:
	pdebug("bdc_updata_loop end\r\n");
	return ret;
}
