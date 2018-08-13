#ifndef _IFLASH_H_
#define _IFLASH_H_

#include "datatype.h"

#define IFLASH_PAGE_SIZE	1024
#define IFLASH_PAGE_NUM		128

void IFLASH_Lock(void);
void IFLASH_Unlock(void);
INT32 IFLASH_Erase(UINT32 page_addr, INT32 page_num);
INT32 IFLASH_Write(UINT32 addr, UINT8 *src, INT32 len);
INT32 IFLASH_Read(UINT32 addr, UINT8 *dst, INT32 len);

#endif
