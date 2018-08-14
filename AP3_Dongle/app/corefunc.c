#include <string.h>
#include "xmodem.h"
#include "corefunc.h"
#include "core.h"
#include "event.h"
#include "flash.h"
#include "debug.h"
#include "corefunc.h"
#include "protocol.h"


void Core_ParseFlashData(UINT32 addr)
{
	UINT16 cmd = 0;
	
	Flash_Read(addr+4, (UINT8 *)&cmd, sizeof(cmd));
	
	if(cmd == CORE_CMD_FW_DATA)
	{
		Event_Set(EVENT_FW_UPDATA);
	}
	else
	{
		Event_Set(EVENT_ESL_UPDATA);
	}
}



UINT8 Core_MallocFlash(UINT32 *addr, UINT32 size)
{
	*addr = Flash_Malloc(size);
	if(*addr == MALLOC_FAIL)
	{
		perr("Core_MallocFlash, size=%d!\r\n", size);
		return 0;
	}
	else
	{
		pdebug("Core_MallocFlash addr=0x%08X, size=%d!\r\n", *addr, size);
		return 1;
	}
}

UINT8 Core_RecvDataToFlash(UINT32 addr, UINT32 len)
{
	sn_t x;
	memset(&x, 0, sizeof(sn_t));
	
	if(protocol_recvToFlash(&x, addr, len, 5000) >= len)
	{
		pdebug("Core_RecvDataToFlash() ok.\r\n");
		return 1;		
	}
	else
	{
		perr("Core_RecvDataToFlash() fail!\r\n");
		return 0;
	}
}

UINT8 Core_SendDataFromFlash(UINT32 addr, UINT32 len)
{
	sn_t x;
	
	memset(&x, 0, sizeof(sn_t));
	
	if(protocol_sendFromFlash(&x, addr, len, 5000) >= len)
	{
		pdebug("Core_SendDataFromFlash() ok.\r\n");
		return 1;		
	}
	else
	{
		perr("Core_SendDataFromFlash() fail!\r\n");
		return 0;
	}
}

UINT8 Core_SendData(UINT8 *src, UINT32 len)
{
	sn_t x;
	
	memset(&x, 0, sizeof(sn_t));
	
	if(protocol_send(&x, src, len, 5000) >= len)
	{
		pdebug("Core_SendData() ok.\r\n");
		return 1;		
	}
	else
	{
		perr("Core_SendData() fail!\r\n");
		return 0;
	}
}

UINT8 Core_CheckBinData(UINT32 addr)
{
//	UINT32 bin_len = 0;
//	UINT32 bin_addr = 0;
//
//	Flash_Read(addr+4+2, (UINT8 *)&bin_len, sizeof(bin_len));
//	bin_addr = addr+4+2+4;
//	pdebug("core check bin, addr = 0x%08X, len = %d.\r\n", bin_addr, bin_len);
//	return check_bin_data(bin_addr, bin_len);
    return 0;
}

UINT8 Core_SetBootData(UINT32 addr)
{
//	UINT32 bin_len = 0;
//	UINT32 bin_addr = 0;
//	UINT32 free_addr = 0;
	UINT8 ret = 0;
	
//	Flash_Read(addr+4+2, (UINT8 *)&bin_len, sizeof(bin_len));
//	bin_addr = addr+4+2+4;
//	pdebug("Core_SetBootData, bin_addr = 0x%08X, bin_len = %d.\r\n", bin_addr, bin_len);
//	if((free_addr=get_free_addr()) == 0)
//	{
//		pdebug("error: get free addr = 0x%08X.\r\n", free_addr);
//		goto done;
//	}
//
//	if(!set_boot_data(free_addr, 0, bin_addr, bin_len))
//	{
//		pdebug("error: set_boot_data.\r\n");
//		goto done;
//	}
	ret = 1;
//done:
	return ret;
}

INT32 Core_MakeCmdBuf(UINT16 cmd, UINT8 *cmd_data, INT32 data_len, UINT8 *dst, INT32 dst_size)
{
	INT32 cmd_len = 0;
	
	memcpy(dst, &cmd, sizeof(cmd));
	cmd_len = dst == NULL ? 0 : data_len;
	memcpy(dst+sizeof(cmd), &cmd_len, sizeof(cmd_len));
	if(dst != NULL)
	{
		memcpy(dst+sizeof(cmd)+sizeof(cmd_len), cmd_data, cmd_len);
	}
	
	return dst == NULL ? sizeof(cmd)+sizeof(cmd_len) : sizeof(cmd)+sizeof(cmd_len)+data_len; 
}

