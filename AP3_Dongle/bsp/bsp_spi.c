/******************************************************************************

 @file       bsp_spi.c

 @brief Common API for SPI access. Driverlib implementation.

 Group: CMCU, SCS
 Target Device: CC2640R2

 ******************************************************************************
 
 Copyright (c) 2014-2017, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: simplelink_cc2640r2_sdk_1_40_00_45
 Release Date: 2017-07-20 17:16:59
 *****************************************************************************/
#include "bsp.h"
#include <bsp_spi.h>
#include <driverlib/ssi.h>
#include <driverlib/gpio.h>
#include <driverlib/prcm.h>
#include <driverlib/ioc.h>
#include <driverlib/rom.h>
#include "CC2640R2_LAUNCHXL.h"
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
/* Board specific settings for CC26xx SensorTag, PCB version 1.01
 *
 * Note that since this module is an experimental implementation,
 * board specific settings are directly hard coded here.
 */
#define BLS_SPI_BASE      SSI1_BASE
#define BLS_CPU_FREQ      48000000ul
#define BSP_SPI_MISO    CC2640R2_LAUNCHXL_SPI1_MISO
#define BSP_SPI_MOSI    CC2640R2_LAUNCHXL_SPI1_MOSI
#define BSP_SPI_CLK     CC2640R2_LAUNCHXL_SPI1_CLK
#define BSP_SPI_CSN     CC2640R2_LAUNCHXL_SPI1_CSN


Power_NotifyObj        spiPostObj;         /*!< SPI post-notification object */
static int spiPostNotify(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg);
/**
 * Write a command to SPI
 */
int bspSpiWrite(const uint8_t *buf, uint16_t len)
{
  while (len > 0)
  {
//    uint32_t ul;

    SSIDataPut(BLS_SPI_BASE, *buf);
//    SSIDataGet(BLS_SPI_BASE, &ul);
    len--;
    buf++;
  }

  return (0);
}

/**
 * Read from SPI
 */
int bspSpiRead(uint8_t *buf, uint16_t len)
{
  while (len > 0)
  {
    uint32_t ul;

    if (!SSIDataPutNonBlocking(BLS_SPI_BASE, 0))
    {
      /* Error */
      return (-1);
    }

    SSIDataGet(BLS_SPI_BASE, &ul);
    *buf = (uint8_t) ul;
    len--;
    buf++;
  }

  return (0);
}


/* See bsp_spi.h file for description */
void bspSpiFlush(void)
{
  uint32_t ul;

  while (SSIDataGetNonBlocking(BLS_SPI_BASE, &ul));
}


/* See bsp_spi.h file for description */
static void bspSpiInit(uint32_t bitRate)
{
    /* GPIO power && SPI power domain */
    PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH | PRCM_DOMAIN_SERIAL);
    while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH | PRCM_DOMAIN_SERIAL)
           != PRCM_DOMAIN_POWER_ON);

    /* GPIO power */
    PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);
    PRCMLoadSet();
    while (!PRCMLoadGet());

    /* SPI power */
    PRCMPeripheralRunEnable(PRCM_PERIPH_SSI1);
    PRCMLoadSet();
    while (!PRCMLoadGet());

    /* SPI configuration */
    SSIIntDisable(BLS_SPI_BASE, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
    SSIIntClear(BLS_SPI_BASE, SSI_RXOR | SSI_RXTO);
    SSIConfigSetExpClk(BLS_SPI_BASE,
                       BLS_CPU_FREQ, /* CPU rate */
                       SSI_FRF_MOTO_MODE_1, /* frame format */
                       SSI_MODE_MASTER, /* mode */
                       bitRate, /* bit rate */
                       8); /* data size */
    IOCPinTypeSsiMaster(BLS_SPI_BASE, IOID_UNUSED, BSP_SPI_MOSI,
                        BSP_SPI_CSN /* csn */, BSP_SPI_CLK);
    SSIEnable(BLS_SPI_BASE);

    {
      /* Get read of residual data from SSI port */
      uint32_t buf;

      while (SSIDataGetNonBlocking(BLS_SPI_BASE, &buf));
    }

}
void bspSpiOpen(uint32_t bitRate)
{
    bspSpiInit(bitRate);
    Power_registerNotify(&spiPostObj, PowerCC26XX_AWAKE_STANDBY, (Power_NotifyFxn)spiPostNotify, bitRate);

}

/* See bsp_spi.h file for description */
void bspSpiClose(void)
{
  // Power down SSI1
  PRCMPeripheralRunDisable(PRCM_PERIPH_SSI1);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  PRCMPeripheralRunDisable(PRCM_PERIPH_GPIO);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  PRCMPowerDomainOff(PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH);
  while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH)
         != PRCM_DOMAIN_POWER_OFF);

}

/*
 *  ======== spiPostNotify ========
 *  This functions is called to notify the SPI driver of an ongoing transition
 *  out of sleep mode.
 *
 *  @pre    Function assumes that the SPI handle (clientArg) is pointing to a
 *          hardware module which has already been opened.
 */
static int spiPostNotify(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
#ifdef SPICC26XXDMA_WAKEUP_ENABLED
    SPICC26XXDMA_Object *object;
#endif


#ifdef SPICC26XXDMA_WAKEUP_ENABLED
    object = spiHandle->object;
#endif

    /* Reconfigure the hardware when returning from standby */
    bspSpiInit(clientArg);

#ifdef SPICC26XXDMA_WAKEUP_ENABLED
    /* check if waken up by CSN */
    /* PIN_hwi() is called after this in whick GPIO_O_EVFLAGS31_0 is cleared */
    /* Therefore must check it here, not in SPICC26XXDMA_csnCallback */
    if (HWREG(GPIO_BASE + GPIO_O_EVFLAGS31_0) == (0x1 << object->csnPin)) {
        /* Call user defined wake up callback */
        if (object->wakeupCallbackFxn) {
            object->wakeupCallbackFxn(spiHandle);
        }
    }
#endif

    return Power_NOTIFYDONE;
}
