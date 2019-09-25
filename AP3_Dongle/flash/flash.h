#ifndef __FLASH_H_
#define __FLASH_H_


#include "datatype.h"

#define FLASH_INIT_ERR		0
#define FLASH_INIT_OK		1
#define FLASH_INIT_ERR_IO	2
#define FLASH_INIT_ERR_CK	3

#define MALLOC_FAIL			0xFFFFFFFF
//#define FLASH_APP_TEST
#define SECTOR_ERR			0x55
#define SECTOR_OK			0xFF
#define FLASH_USE_FLAG		0xAA
#define FLASH_BASE_ADDR		0x00

extern BOOL Flash_SetErrorSector(UINT16 sector);
extern UINT8 Flash_GetSectorStatus(UINT16 sector);
//flash初始化函数，包括IO初始化和sector check，成功返回OK，失败返回失败码
extern UINT8 Flash_Init(void);
//软复位
extern void Flash_SoftReset(void);
//在flash中分配size大小的空间，成功返回分配空间的起始地址，失败返回MALLOC_FAIL
extern UINT32 Flash_Malloc(UINT32 size);
//在flash的addr地址写起始地址为src，长度为len的数据，addr可为任意地址，但必须是已经擦除的区域。
extern BOOL Flash_Write(UINT32 addr, UINT8* src, UINT32 len);
//在flash的addr地址读取长度为len的数据到dst，addr为任意地址
extern BOOL Flash_Read(UINT32 addr, UINT8* dst, UINT32 len);

extern BOOL Flash_writeInfo(UINT8* src, UINT32 len);
extern BOOL Flash_readInfo(UINT8* src, UINT32 len);
extern UINT8 Flash_calibInfoInit(void);
extern void test_flash(void);
#endif


