#ifndef _CORE_FUNC_H
#define _CORE_FUNC_H
#include "datatype.h"

void Core_ParseFlashData(UINT32 addr);
UINT8 Core_MallocFlash(UINT32 *addr, UINT32 size);
UINT8 Core_RecvDataToFlash(UINT32 addr, UINT32 len);
UINT8 Core_SendDataFromFlash(UINT32 addr, UINT32 len);
UINT8 Core_SendData(UINT8 *src, UINT32 len);
UINT8 Core_CheckBinData(UINT32 addr);
UINT8 Core_SetBootData(UINT32 addr);
INT32 Core_MakeCmdBuf(UINT16 cmd, UINT8 *cmd_data, INT32 data_len, UINT8 *dst, INT32 dst_size);

#endif
