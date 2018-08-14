/*
 * SPI.c
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */

#include <ti/drivers/spi/SPICC26XXDMA.h>
#include "Dongle_SPI.c"

static void transferCallback(SPI_Handle handle, SPI_Transaction *transaction)
{
    uint8_t i;
    uint8_t *p = transaction->rxBuf;
    // Start another transfer
    SPI_transfer(handle, transaction);
    for (i=0; i<sizeof(buf); i++){
        p[i] = 0;
    }
}

void SPI_Init(UArg a0, UArg a1)
{
    SPI_Handle handle;
    SPI_Params params;
    SPI_Transaction transaction;

    // Init SPI and specify non-default parameters
    SPI_Params_init(&params);
    params.bitRate             = 1000000;
    params.frameFormat         = SPI_POL1_PHA1;
    params.mode                = SPI_SLAVE;
    params.transferMode        = SPI_MODE_CALLBACK;
    params.transferCallbackFxn = transferCallback;
    // Configure the transaction
    transaction.count = BUF_LEN;
    transaction.txBuf = spi_tx_buf;
    transaction.rxBuf = spi_rx_buf;
    // Open the SPI and initiate the first transfer
    handle = SPI_open(Board_SPI0, &params);
    SPI_control(handle, SPICC26XXDMA_RETURN_PARTIAL_ENABLE, NULL);
    SPI_transfer(handle, &transaction);

//    // Wait forever
//    while(true){
//        System_printf("%d, %d", transaction.status, transaction.count);
//    }
}

void UART_appSPIWrite()
{
    //write data
    //pull down io
}
