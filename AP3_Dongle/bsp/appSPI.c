/*
 * SPI.c
 *
 *  Created on: 2018Äê8ÔÂ13ÈÕ
 *      Author: ggg
 */
#include "Board.h"
#include "appSPI.h"


#define SPI_RATE    4000000
#define SPI_BUF_LEN 512

extern void transferCallback(SPI_Handle handle, SPI_Transaction *transaction);

void SPI_appInit(uint8_t* rxbuf, uint8_t* txbuf)
{
    SPI_Handle handle;
    SPI_Params params;
    SPI_Transaction transaction;

    // Init SPI and specify non-default parameters
    SPI_Params_init(&params);
    params.bitRate             = SPI_RATE;
    params.frameFormat         = SPI_POL0_PHA0;
    params.mode                = SPI_SLAVE;
    params.transferMode        = SPI_MODE_CALLBACK;
    params.transferCallbackFxn = transferCallback;
    // Configure the transaction
    transaction.count = SPI_BUF_LEN;
    transaction.txBuf = txbuf;
    transaction.rxBuf = rxbuf;
    // Open the SPI and initiate the first transfer
    handle = SPI_open(Board_SPI1, &params);
//    SPI_control(handle, SPICC26XXDMA_RETURN_PARTIAL_ENABLE, NULL);
    SPI_transfer(handle, &transaction);

}

void SPI_appSend()
{

}

void SPI_appRecv()
{

}

