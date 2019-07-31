#include "data.h"
#include "updata.h"
#include <stdio.h>
#include <string.h>
#include "flash.h"
#include "crc16.h"
#include "debug.h"

INT32 get_updata_para(UINT32 addr, void *dst)
{
	Flash_Read(addr, (UINT8 *)dst, LEN_OF_UPDATA_PARA);
	return LEN_OF_UPDATA_PARA;
}

INT32 get_frame1_para(UINT32 addr, void *dst)
{
	Flash_Read(addr, (UINT8 *)dst, LEN_OF_FRAME1_PARA);
	return LEN_OF_FRAME1_PARA;
}

UINT16 g3_get_wkup_interval(UINT32 wkup_start_addr)
{
	UINT16 interval = 0;
	Flash_Read(wkup_start_addr+6, (UINT8 *)&interval, sizeof(interval));
	return interval;
}

UINT16 g3_get_cmd(UINT32 addr, UINT32 *cmd_len)
{	
	UINT16 cmd = 0;
	
	Flash_Read(addr, (UINT8 *)&cmd, sizeof(cmd));
	if(cmd_len != NULL)
	{
		Flash_Read(addr+sizeof(cmd), (UINT8 *)cmd_len, sizeof(*cmd_len));
	}
	
	return cmd;
}

UINT32 g3_get_sid(UINT32 addr)
{
	UINT32 sid = 0;

	if(Flash_Read(addr, (void *)&sid, 4) == FALSE)
	{
		return 0;
	}
	else
	{
		return sid;
	}
}
INT32 get_flash_led_data(UINT32 addr, void *data, UINT16 len)
{
#ifdef FLASH_LED_TEST
	uint8_t i = 0, *p = data;
	p[0] = 2;
	p[1] = 0;
	p[2] = 0x50;
	p[3] = 0x81;
	p[4] = 0x01;
	p[5] = 0x66;
	p[6] = 167;
	p[7] = 26;

	p[8] = 0x11;	//ctrl+color
	for (i=9;i<9+20;i++){
		p[i] = 0xff;
	}
	p[29] = 0x0;	//page
	p[30] = 0x0;	//page
	p[31] = 0x0;	//reserve
	p[32] = 0x0;	//crc
	p[33] = 0x0;	//crc

	p[34] = 0x50;
	p[35] = 0x81;
	p[36] = 0x01;
	p[37] = 0x66;
	p[38] = 167;
	p[39] = 26;

	p[40] = 0x10;	//ctrl
	for (i=41;i<41+20;i++){
		p[i] = 0xff;
	}
	p[61] = 0x0;	//page
	p[62] = 0x0;	//page
	p[63] = 0x0;	//reserve
	p[64] = 0x0;	//crc
	p[65] = 0x0;	//crc
	return 1;
#else
	return Flash_Read(addr, data, len);
#endif
}
INT32 get_one_data(UINT32 addr, UINT8 *id, UINT8 *ch, UINT8 *len, UINT8 *dst, UINT8 size)
{
	INT32 ret = 0;
	UINT8 head[6] = {0};    //group id[4],channel, length
	
	Flash_Read(addr, head, 6);
	
	if(id != NULL)
	{
		memcpy(id, head, 4);
	}

	if(ch != NULL)
	{
		*ch = head[4];
	}

	if(len != NULL)
	{
		*len = head[5];
	}

	if(dst != NULL)
	{		
		if(head[5] > size)
		{
			ret = -1;
			goto done;
		}
		
		Flash_Read(addr+6, dst, head[5]);
	}

	ret = 6+head[5];

done:
	return ret;
}

UINT16 get_pkg_sn_f(UINT32 pkg_addr, UINT8 sn_offset)
{
	UINT16 sn = 0;
	Flash_Read(pkg_addr+sn_offset, (UINT8 *)&sn, 2);
	return (sn & MASK_OF_PKG_SN);
}

#if 1

//extern INT32 search_pkg_sn_times;
//extern UINT16 search_pkg_history[40];
//extern INT32 search_end_pkg;

UINT32 get_pkg_addr_bsearch(UINT32 first_pkg_addr, UINT16 pkg_num, UINT16 target_pkg_sn, UINT8 sn_offset)
{
	UINT16 fsn = 0;
	UINT32 ret = 0;
	INT32 start=0, mid = 0, end=pkg_num-1;
	UINT32 addr = 0;
	
//	search_end_pkg = end;
	
	while(start <= end)
	{
		mid = (start + end) / 2;
		addr = first_pkg_addr+mid * SIZE_ESL_DATA_SINGLE;
		Flash_Read(addr+sn_offset, (UINT8 *)&fsn, sizeof(fsn));
		fsn &= MASK_OF_PKG_SN;
		
		//for debug
//		search_pkg_history[search_pkg_sn_times] = fsn;
//		search_pkg_sn_times++;
		
		if(fsn > target_pkg_sn)
		{
			end = mid - 1;
		}
		else if(fsn < target_pkg_sn)
		{
			start = mid + 1;
		}
		else // ==
		{
			ret = addr;
			break;
		}
	}
	
	return ret;
}

#else 
UINT32 get_pkg_addr_bsearch(UINT32 first_pkg_addr, INT32 start, INT32 end, UINT16 target_pkg_sn, UINT8 sn_offset)
{
	UINT16 fsn = 0;
	UINT32 ret = 0;
	INT32 mid = 0;
	
	while(start <= end)
	{
		mid = (start + end) / 2;	
		Flash_Read(first_pkg_addr+mid*32+sn_offset, (UINT8 *)&fsn, 2);
		fsn &= MASK_OF_PKG_SN;
		if(fsn > target_pkg_sn)
		{
			end = mid - 1;
		}
		else if(fsn < target_pkg_sn)
		{
			start = mid + 1;
		}
		else // ==
		{
			ret = first_pkg_addr+mid*32;
			break;
		}
	}
	
	return ret;
}
#endif

UINT8 g3_get_wkup_para(UINT32 addr, UINT16 *datarate, UINT8 *power, UINT8 *duration, UINT8 *slot_duration, UINT8 *mode)
{
	if(datarate != NULL)
	{
		Flash_Read(addr, (UINT8 *)datarate, 2);
	}
	
	if(power != NULL)
	{
		Flash_Read(addr+2, power, 1);
	}

	if(duration != NULL)
	{
		Flash_Read(addr+3, (UINT8 *)duration, 1);
	}
	
	if(slot_duration != NULL)
	{
		Flash_Read(addr+4, slot_duration, 1);
	}
	
	if(mode != NULL)
	{
		Flash_Read(addr+6, mode, 1);
	}
	
	return 1;
}

INT32 g3_get_wkup_loop_times(UINT32 addr)
{
	UINT8 times = 0;
	
	Flash_Read(addr+5, &times, 1);
	
	return times;
}

UINT8 g3_set_ack_para(UINT32 addr, UINT32 sid, UINT16 cmd, UINT32 cmd_len, UINT8 para, UINT16 num)
{
	UINT32 offset = addr;
	UINT8 ret = 0;
	
	if(Flash_Write(offset, (UINT8 *)&sid, sizeof(sid)) == FALSE)
	{
		goto done;
	}
	offset += sizeof(sid);
	
	if(Flash_Write(offset, (UINT8 *)&cmd, sizeof(cmd)) == FALSE)
	{
		goto done;
	}
	offset += sizeof(cmd);
	
	if(Flash_Write(offset, (UINT8 *)&cmd_len, sizeof(cmd_len)) == FALSE)
	{
		goto done;
	}
	offset += sizeof(cmd_len);
	
	if(Flash_Write(offset, (UINT8 *)&para, sizeof(para)) == FALSE)
	{
		goto done;
	}
	offset += sizeof(para)+8;
	
	if(Flash_Write(offset, (UINT8 *)&num, sizeof(num)) == FALSE)
	{
		goto done;
	}
//	offset += sizeof(num);
	
	ret = 8;
done:
	return ret;
}

UINT8 g3_set_ack_crc(UINT32 addr, UINT32 len)
{
	UINT8 buf[256] = {0};
	UINT16 buf_len = 0;
	UINT16 crc = 0;
	INT32 left_len = len;
	UINT32 offset = addr;
	UINT8 ret = 0;
	
	while(left_len > 0)
	{
		buf_len = sizeof(buf) > left_len ? left_len : sizeof(buf);
		if(Flash_Read(offset, buf, buf_len) == FALSE)
		{
			break;
		}
		crc = CRC16_CaculateStepByStep(crc, buf, buf_len);
		left_len -= buf_len;
		offset += buf_len;
//		buf_len = 0;
	}
	
	if(left_len <= 0)
	{
		if(Flash_Write(addr+len, (UINT8 *)&crc, sizeof(crc)) == TRUE)
		{
			ret = 1;
		}
	}
		
	return ret;
}

UINT8 g3_set_ack(UINT32 addr, UINT8 *id, UINT8 ack)
{
	UINT32 offset = addr;
	UINT8 ret = 0;
	
	if(Flash_Write(offset, id, 4) == FALSE)
	{
		goto done;
	}
	offset += 4;

	if(Flash_Write(offset, &ack, 1) == FALSE)
	{
		goto done;
	}
//	offset += 4;
	
	ret = 1;
	
done:
	return ret;
}

UINT8 g3_set_new_ack(UINT32 addr, UINT8 *id, UINT8 ack, UINT8 *detail, UINT8 detail_len)
{
	UINT32 offset = addr;
	UINT8 ret = 0;
	
	if(Flash_Write(offset, id, 4) == FALSE)
	{
		goto done;
	}
	offset += 4;

	if(Flash_Write(offset, &ack, 1) == FALSE)
	{
		goto done;
	}
	offset += 1;
	
	if(Flash_Write(offset, detail, detail_len) == FALSE)
	{
		goto done;
	}
	//offset += detail_len;
	
	ret = 1;
done:
	return ret;
}

UINT32 g3_print_ctrl = 0;

void g3_set_print(UINT8 enable)
{
	g3_print_ctrl = enable;
}

void g3_print_data(UINT32 addr, UINT32 len)
{
if(g3_print_ctrl != 0)
{
	
	INT32 i;
	INT32 left_len = len;
	UINT8 buf[256] = {0};
	UINT16 buf_len = 0;
	UINT32 offset = addr;
	
	log_print("DATA\r\n");
	while(left_len > 0)
	{
		buf_len = left_len > sizeof(buf) ? sizeof(buf) : left_len;
		if(Flash_Read(offset, buf, buf_len) == FALSE)
		{
			break;
		}
		
		left_len -= buf_len;
		offset += buf_len;
		
		for(i = 0; i < buf_len; i++)
		{
			if((left_len<=0) && (i==(buf_len-1)))
			{
			    log_print("0x%02X.\r\n", buf[i]);
			}
			else
			{
			    log_print("0x%02X,", buf[i]);
			}
		}
	}
	
	log_print("\r\n");
}
}

void g3_print_ack(UINT32 addr, UINT32 len)
{
if(g3_print_ctrl != 0)
{
		
	INT32 i;
	INT32 left_len = len;
	UINT8 buf[256] = {0};
	UINT16 buf_len = 0;
	UINT32 offset = addr;

	log_print("ACK\r\n");
	while(left_len > 0)
	{
		buf_len = left_len > sizeof(buf) ? sizeof(buf) : left_len;
		if(Flash_Read(offset, buf, buf_len) == FALSE)
		{
			break;
		}
		
		left_len -= buf_len;
		offset += buf_len;
		
		for(i = 0; i < buf_len; i++)
		{
			if((left_len<=0) && (i==(buf_len-1)))
			{
				log_print("0x%02X.\r\n", buf[i]);
			}
			else
			{
				log_print("0x%02X,", buf[i]);
			}
		}
	}
	log_print("\r\n");
}
}

