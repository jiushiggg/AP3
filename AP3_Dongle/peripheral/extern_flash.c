
#include "extern_flash.h"
#include "datatype.h"
#include "Board.h"
#include <ti/drivers/nvs/NVSSPI25X.h>
#include <ti/drivers/GPIO.h>
//#include <ti/drivers/spi/SPICC26XXDMA.h>

#define BLS_CODE_MDID             0x90 /**< Manufacturer Device ID */

#define  EF_BLOCK_SIZE   ((UINT32)0x1000)   //4k
#define  DMA_MAX_BUF    1024
static NVS_Handle nvsHandle;

void init_nvs_spi_flash(void)
{
//    NVS_Attrs regionAttrs;
    NVS_Params nvsParams;

    NVS_init();
    NVS_Params_init(&nvsParams);
    nvsHandle = NVS_open(Board_NVS1, &nvsParams);
    if(!nvsHandle)
    {
        while(1);
    }
}

void extern_flash_open(void)
{
    NVS_Params nvsParams;
    nvsHandle = NVS_open(Board_NVS1, &nvsParams);
}
void extern_flash_close(void)
{
    NVS_close(nvsHandle);           //flash½øÈësleep×´Ì¬
}
ReturnMsg CMD_SE(WORD seg_addr)
{
    NVS_erase(nvsHandle, seg_addr, EF_BLOCK_SIZE);
    return FlashOperationSuccess;
}

ReturnMsg CMD_PP(WORD addr, WORD data, WORD len, UINT8 flg)
{
    uint32_t tmp_len = 0;
    while(len > 0){
        if (len > DMA_MAX_BUF){
            tmp_len = DMA_MAX_BUF;
            len -= DMA_MAX_BUF;
        }else {
            tmp_len = len;
            len = 0;
        }
        //NVS_write(nvsHandle, addr, (void *)data, tmp_len, NVS_WRITE_POST_VERIFY);
        if (NVS_STATUS_SUCCESS == NVS_write(nvsHandle, addr, (void *)data, tmp_len, flg)){
            data += tmp_len;
            addr += tmp_len;
        }else{
            return FlashWriteRegFailed;
        }

    }
    return FlashOperationSuccess;
}

ReturnMsg CMD_FASTREAD(WORD addr, WORD buf, WORD len)
{
    uint32_t tmp_len = 0;
    while(len > 0){
        if (len > DMA_MAX_BUF){
            tmp_len = DMA_MAX_BUF;
            len -= DMA_MAX_BUF;
        }else {
            tmp_len = len;
            len = 0;
        }
        NVS_read(nvsHandle, addr, (void *)buf,tmp_len);
        buf += tmp_len;
        addr += tmp_len;
    }

    return FlashOperationSuccess;
}

static void extFlashSelect(void)
{
    GPIO_write(((NVSSPI25X_HWAttrs*)nvsHandle->hwAttrs)->spiCsnGpioIndex, 0);
    //PIN_setOutputValue(hFlashPin,Board_SPI_FLASH_CS,Board_FLASH_CS_ON);
}

static void extFlashDeselect(void)
{
    GPIO_write(((NVSSPI25X_HWAttrs*)nvsHandle->hwAttrs)->spiCsnGpioIndex, 0);
}

UINT32 CMD_RDID(void)
{
    uint8_t wbuf[] = { BLS_CODE_MDID, 0xFF, 0xFF, 0x00 };
    uint8_t infoBuf[2]={0};
    SPI_Transaction transaction;

    extFlashSelect();
    // Configure the transaction
    transaction.count = sizeof(wbuf);
    transaction.txBuf = (uint8_t*)wbuf;
    transaction.rxBuf = NULL;
    SPI_transfer(((NVSSPI25X_Object*)nvsHandle->object)->spiHandle, &transaction);

    transaction.count = sizeof(infoBuf);
    transaction.txBuf = NULL;
    transaction.rxBuf = infoBuf;
    SPI_transfer(((NVSSPI25X_Object*)nvsHandle->object)->spiHandle, &transaction);
    extFlashDeselect();

    return (uint16_t)infoBuf[0]<<8 | infoBuf[1];
}

