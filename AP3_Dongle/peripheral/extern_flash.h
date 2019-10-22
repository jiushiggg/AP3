
#ifndef  EXTERN_FLASH_H
#define  EXTERN_FLASH_H

#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSSPI25X.h>
#include "datatype.h"

// Return Message
typedef enum {
    FlashOperationSuccess,
    FlashWriteRegFailed,
    FlashTimeOut,
    FlashIsBusy,
    FlashQuadNotEnable,
    FlashAddressInvalid,
}ReturnMsg;

#define    FlashID          0xc214
#define    FlashID_GD       0xc814
#define    FlashID_PUYA     0x8514
#define    FlashID_WB		0xEF14

#define VERIFY      NVS_WRITE_POST_VERIFY
#define NOT_VERIFY  0
extern void init_nvs_spi_flash(void);
extern void extern_flash_open(void);
extern void extern_flash_close(void);
extern ReturnMsg CMD_SE(WORD seg_addr);
extern ReturnMsg CMD_PP(WORD addr, WORD data, WORD len, UINT8 flg);
extern ReturnMsg CMD_FASTREAD(WORD addr, WORD buf, WORD len);
extern UINT32 CMD_RDID(void);

#endif
