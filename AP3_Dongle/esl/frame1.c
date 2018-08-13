#include "cc2640r2_rf.h"
#include "frame1.h"
#include "flash.h"
#include "timer.h"
#include "debug.h"
#include "updata.h"
#include "data.h"
#include "crc16.h"
#include <string.h>
#include "core.h"
#include "bsp.h"

#pragma pack(1)
typedef struct
{
	UINT16 datarate;
	UINT8 power;
	UINT8 duration;
	UINT8 mode;
	UINT8 slot;
	UINT8 reserved[7];
	UINT16 num;
}frame1_para_t;
#pragma pack()

static frame1_para_t frame1_para;

static UINT8 frame1_mode0(UINT32 addr, UINT8 num, INT32 duration)
{
	UINT8 ret = 0;
	UINT8 id[4] = {0};
	UINT8 channel = 0;
	UINT8 data[30] = {0};
	UINT8 len = 0;
	INT32 i = 0;
	UINT8 timer = 0;
	UINT32 cur = 0;
	RF_EventMask result;
	uint8_t pend_flg = PEND_STOP;
	
	if((timer=TIM_Open(duration, 1, TIMER_UP_CNT, TIMER_ONCE)) == TIMER_UNKNOW)
	{
		perr("g3_send_frame1_mode0() open timer.\r\n");
		goto done;
	}
	
	i = 0;
	cur = addr;
	while(1)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("frame1_mode0 quit1\r\n");
			break;
		}
		
		if(TIM_CheckTimeout(timer)==TIME_OUT)
		{
			pdebug("g3_send_frame1_mode0() timer timeout.\r\n");
			ret = 1;
			break;
		}
	
		if(get_one_data(cur, id, &channel, &len, data, sizeof(data)) == 0)
		{
			perr("g3_send_frame1_mode0() get data.\r\n");
			break;
		}
		
		/*
		pdebug("g3_send_frame1_mode0 frame1 %d: id=0x%02X-0x%02X-0x%02X-0x%02X, channel=%d, len=%d, data=", \
				i, id[0], id[1], id[2], id[3], channel, len);
		pdebughex(data, len);
		*/
        if (PEND_START == pend_flg){
            send_pend(result);
        }
		result = send_without_wait(id, data, len, channel, 6000);
        pend_flg = PEND_START;
		
		i += 1;		
		if(i == num)
		{
			i = 0;
			cur = addr;
		}
		else
		{
			cur += sizeof(id) + sizeof(len) + len + sizeof(channel);
		}
	}
	TIM_Close(timer);
	wait(1000);
	
done:	
	return ret;
}

static UINT8 _frame2(UINT32 addr, UINT8 num, INT32 duration)
{
	UINT8 ret = 0;
	UINT8 id[4] = {0};
	UINT8 channel = 0;
	UINT8 data[30] = {0};
	UINT8 len = 0;
	INT32 i = 0;
	UINT8 timer = 0;
	UINT32 cur = 0;
	UINT16 timercount=0, crc = 0;
    RF_EventMask result;
    uint8_t pend_flg = PEND_STOP;
	
	if((timer=TIM_Open(10, duration/10, TIMER_DOWN_CNT, TIMER_PERIOD)) == TIMER_UNKNOW) //TODO: mode right?
	{
		perr("frame2_mode0() open timer.\r\n");
		goto done;
	}
	
	i = 0;
	cur = addr;
	while(1)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("_frame2 quit1\r\n");
			break;
		}		
		
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			pdebug("_frame2() timeout\r\n");
			ret = 1;
			break;		
		}
		
		if(get_one_data(cur, id, &channel, &len, data, sizeof(data)) == 0)
		{
			perr("_frame2() get data\r\n");
			break;
		}
		
		timercount = TIM_GetCount(timer);
		memcpy(data+2, &timercount, sizeof(timercount));
		crc = 0;
		crc = CRC16_CaculateStepByStep(crc, data, 24);
		crc = CRC16_CaculateStepByStep(crc, id, 4);
		memcpy(data+24, &crc, sizeof(crc));
		
		/*
		pdebug("_frame2 %d: id=0x%02X-0x%02X-0x%02X-0x%02X, channel=%d, len=%d, data=", \
				i, id[0], id[1], id[2], id[3], channel, len);
		pdebughex(data, len);
		*/
        if (PEND_START == pend_flg){
            send_pend(result);
        }
        result = send_without_wait(id, data, len, channel, 6000);
        pend_flg = PEND_START;
		
		i += 1;		
		if(i == num)
		{
			i = 0;
			cur = addr;
		}
		else
		{
			cur += sizeof(id) + sizeof(len) + len + sizeof(channel);
		}
	}

	TIM_Close(timer);
	wait(1000);
	
done:	
	return ret;
}


INT32 frame1_start(UINT16 cmd, UINT32 addr, UINT32 len)
{
	INT32 ret = 0;
	INT32 dur = 0;

	if((addr==0) || (len==0))
	{
		ret = -1;
		goto done;
	}

	get_frame1_para(addr, &frame1_para);
	addr += LEN_OF_FRAME1_PARA;
	len -= LEN_OF_FRAME1_PARA;

	if(cmd == CMD_GROUP1_FRAME2)
	{	
		dur = frame1_para.duration * 1000;//in frame2 unit is s
	}
	else
	{
		dur = frame1_para.duration;
	}
	
	pdebug("frame1:datarate=%d,power=%d,duration=%dms,mode=%d,num=%d\r\n",
			frame1_para.datarate, frame1_para.power, dur,
			frame1_para.mode, frame1_para.num);
	
	if(frame1_para.duration == 0)
	{
		pdebug("warning: frame 1 duration is 0\r\n");
		goto done;
	}
	
	set_power_rate(frame1_para.power, frame1_para.datarate);
	
	if(cmd == CMD_GROUP1_FRAME2)
	{
		ret = _frame2(addr, frame1_para.num, dur);
	}
	else
	{
		switch(frame1_para.mode)
		{
			case 0: // 0 is default
			default:
				if(frame1_mode0(addr, frame1_para.num, dur) == 1)
				{
					ret = 1;
				}
			break;
		}
	}
	
done:
	pdebug("frame1 end\r\n");
	return ret;
}

INT32 frame1_dummy(UINT32 addr, UINT32 len, UINT32 *dummy_offset, INT32 dummy_num)
{
	INT32 ret = 0;
	INT32 tx_num = 0;

	UINT8 id[4] = {0};
	UINT8 channel = 0;
	UINT8 data[30] = {0};
	UINT8 data_len = 0;
    RF_EventMask result;
    uint8_t pend_flg = PEND_STOP;

	if((addr==0) || (len==0))
	{
		ret = -1;
		goto done;
	}

	get_frame1_para(addr, &frame1_para);
	
	pdebug("frame1 dummy:datarate=%d,power=%d,duration=%d,mode=%d,num=%d.\r\n", 
			frame1_para.datarate, frame1_para.power, frame1_para.duration,
			frame1_para.mode, frame1_para.num);
	
	set_power_rate(frame1_para.power, frame1_para.datarate);

	
	while(tx_num < dummy_num)
	{	
		if((*dummy_offset == 0) || (*dummy_offset >= (addr+len)))
		{
			*dummy_offset = addr+LEN_OF_FRAME1_PARA;
		}

		if(get_one_data(*dummy_offset, id, &channel, &data_len, data, sizeof(data)) == 0)
		{
			perr("frame1 dummy get data\r\n");
			break;
		}
		
		pdebug("frame1 dummy: id=0x%02X-0x%02X-0x%02X-0x%02X, channel=%d, len=%d, data=", \
				id[0], id[1], id[2], id[3], channel, data_len);
		pdebughex(data, data_len);

        if (PEND_START == pend_flg){
            send_pend(result);
        }
        result = send_without_wait(id, data, data_len, channel, 6000);
        pend_flg = PEND_START;

		*dummy_offset += sizeof(id)+sizeof(channel)+sizeof(data_len)+data_len;
		tx_num++;
	}

	ret = tx_num;
	wait(1000);	
done:
	return ret;
}
