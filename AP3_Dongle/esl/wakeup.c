#include "cc2640r2_rf.h"
#include "wakeup.h"
#include "flash.h"
#include "timer.h"
#include "debug.h"
#include "data.h"
#include "bsp.h"
#include "core.h"


#define RF_CHANING_MODE
INT32 wakeup_start(UINT32 addr, UINT32 len, UINT8 type)
{
	INT32 ret = 0;
	UINT8 timer = 0;
	UINT32 duration_ms = 0;
	UINT16 timer_count = 0;
	
	UINT8 id[4] = {0};
#ifdef RF_CHANING_MODE
    UINT8 *data = NULL;
#else
    UINT8 data[SIZE_ESL_DATA_BUF] = {0};
#endif
	UINT8 data_len = 0;
	UINT8 power = 0;
	UINT8 channel = 0;
	UINT16 datarate = 0;
	UINT8 duration = 0;
	UINT8 slot_duration = 0;
	UINT8 ctrl = 0;
	UINT16 interval = 0;
	UINT8 mode = 0;
	uint32_t rf_time = 0;
	RF_EventMask result;
	uint8_t  pend_flg = PEND_STOP;
#ifdef RF_CHANING_MODE
	send_chaningmode_init();
    write2buf = listInit();
#endif
	pdebug("wkup addr=0x%08X, len=%d\r\n", addr, len);

	if((addr==0) || (len==0))
	{
		ret = -1;
		goto done;
	}

	if(g3_get_wkup_para(addr, &datarate, &power, &duration, &slot_duration, &mode) == 0)
	{
		perr("g3_wkup() get para from flash.\r\n");
		ret = -2;
		goto done;
	}

	interval = g3_get_wkup_interval(addr);
	if(interval == 0)
	{
		interval = 30;
	}

	pdebug("wkup para: datarate=%d, power=%d, duration=%d, slot_duration=%d, interval=%d\r\n", \
			datarate, power, duration, slot_duration, interval);

	if(duration == 0)
	{
		pdebug("warning: wkup duration is 0\r\n");
		goto done;
	}
#ifdef RF_CHANING_MODE
	data = ((MyStruct*)write2buf)->tx->pPkt;
#endif
	if(get_one_data(addr+OFFSET_WKUP_DATA, id, &channel, &data_len, data, SIZE_ESL_DATA_BUF) == 0)
	{
		perr("g3_wkup() get data from flash.\r\n");
		ret = -3;
		goto done;
	}

	pdebug("wkup data: id:0x%02X-0x%02X-0x%02X-0x%02X, channel=%d, len=%d, data=", \
			id[0], id[1], id[2], id[3], channel, data_len);
	pdebughex(data, data_len);

	ctrl = data[0];
		

//#define GGG_DEBUG
#ifdef GGG_DEBUG
    slot_duration = 10;
    duration = 4;
    interval = 23;
    datarate = DATA_RATE_500K;
    id[0] = 52; id[1] = 0x56; id[2] = 0x78; id[3] = 0x53;
    channel = 2;
    data_len = 26;
#endif


    set_power_rate(power, datarate);
    set_frequence(channel);
#ifdef RF_CHANING_MODE
#else
    send_data_init(id, data, data_len, 5000);
#endif
    rf_time = RF_getCurrentTime();
	if(mode == 1)
	{
		duration_ms = (uint32_t)duration * 10;
	}
	else
	{
		duration_ms = (uint32_t)duration * 1000 - 500;
	}
	if (3 == duration){
		duration_ms -= 300;
	}
	
	if((timer=TIM_Open(slot_duration, duration_ms/slot_duration, TIMER_DOWN_CNT, TIMER_PERIOD)) == TIMER_UNKNOW)
	{
		perr("g3_wkup() open timer.\r\n");
		ret = -4;
		goto done;
	}
	LED_TOGGLE(DEBUG_IO0);
	interval = EasyLink_us_To_RadioTime(interval);
	while(TIME_COUNTING==TIM_CheckTimeout(timer))
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("g3_wkup quit\r\n");
			break;
		}
#ifdef RF_CHANING_MODE
		data = ((MyStruct*)write2buf)->tx->pPkt;
#endif
		if(type == 0) // 0 is default
		{
			timer_count = TIM_GetCount(timer);
			
			if(ctrl == 0xAA)
			{
				data[0] = ctrl;
			}
			else
			{
				data[0] = (ctrl&0xE0) | ((timer_count >> 8) & 0x1f);
			}
			
			data[1] = timer_count & 0xff;
		}
		LED_ON(DEBUG_IO1);
		rf_time += interval;

#ifdef RF_CHANING_MODE
        if (PEND_STOP == pend_flg){
            pend_flg = PEND_START;
            result = send_chaningmode(id, data, data_len, 6000);
            memcpy(((MyStruct*)write2buf->next)->tx->pPkt, data, data_len);
			write2buf = List_next(write2buf);
        }else{
            RF_wait_cmd_finish();
            //write2buf = List_next(write2buf);
        }
#else
        if (PEND_START == pend_flg){
            send_pend(result);
        }
        result = send_async(rf_time);
        pend_flg = PEND_START;
#endif
//		if(send_data(id, data, data_len, channel, 5000) != data_len)
//		{
//			perr("g3_wkup() send data.\r\n");
//			ret = -5;
//			break;
//		}

		LED_OFF(DEBUG_IO1);
//		BSP_Delay10US(interval);
	}
#ifdef RF_CHANING_MODE
    RF_wait_cmd_finish(); //Wait for the last packet to be sent
    RF_cancle(result);
#endif
	LED_TOGGLE(DEBUG_IO0);
	TIM_Close(timer);
	
	ret = 1;
done:
	return ret;
}

INT32 wakeup_get_loop_times(UINT32 addr)
{
	return g3_get_wkup_loop_times(addr);
}
