
/*
** bsp.c: config gpio,rcc,isr,nvic
*/

#include "bsp.h"
#include "bsp_config.h"
#include <stdio.h>

#include <inc/hw_prcm.h>
#include <driverlib/prcm.h>



void BSP_GPIO_init(void)
{
//    /* GPIO power up*/
//    PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
//    while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH)
//           != PRCM_DOMAIN_POWER_ON);
//
//    PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);
//    PRCMLoadSet();
//    while (!PRCMLoadGet());

    /* set direction */
//    GPIO_setOutputEnableDio(DEBUG_IO0, GPIO_OUTPUT_ENABLE);
//    GPIO_setDio(DEBUG_IO0);
//    GPIO_setOutputEnableDio(DEBUG_IO1, GPIO_OUTPUT_ENABLE);
//    GPIO_setDio(DEBUG_IO1);
//    GPIO_setOutputEnableDio(DEBUG_IO2, GPIO_OUTPUT_ENABLE);
//    GPIO_setDio(DEBUG_IO2);
//    GPIO_setOutputEnableDio(DEBUG_IO3, GPIO_OUTPUT_ENABLE);
//    GPIO_setDio(DEBUG_IO3);
//    GPIO_setOutputEnableDio(DEBUG_TEST, GPIO_OUTPUT_ENABLE);
//    GPIO_setDio(DEBUG_TEST);
}

void BSP_GPIO_test(uint32_t n)
{
//    GPIO_toggleDio(n);
}

void BSP_highGPIO(uint32_t n)
{
  //  GPIO_setDio(n);
}

void BSP_lowGPIO(uint32_t n)
{
   // GPIO_clearDio(n);
}

void BSP_GPIO_ToggleDebugPin(void)
{

}

/**
 * @fn      powerDownGpio
 *
 * @brief   Powers down the Gpio peripheral. Note: the External Flash close
 *          will do this also.
 *
 * @param   none
 *
 * @return  none
 */
void powerDownGpio(void)
{
    /* GPIO power down */
//    PRCMPeripheralRunDisable(PRCM_PERIPH_GPIO);
//    PRCMLoadSet();
//    while (!PRCMLoadGet());
//
//    PRCMPowerDomainOff(PRCM_DOMAIN_PERIPH);
//    while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH)
//           != PRCM_DOMAIN_POWER_OFF);
}

void BSP_EnterCri(void)
{
//	__set_FAULTMASK(1);//disable all interrupt
}

void BSP_ExitCri(void)
{
//	__set_FAULTMASK(0);//enable all interrupt
}

void BSP_Delay1S(INT32 n)
{
    volatile int d;

	while((n--) > 0)
	{
		for(d = 0; d < 12000000; d++);
	}
}

void BSP_Delay1MS(INT32 delayMs)
{
    volatile uint32_t j;
    /* by experimination, this is in ms (approx) */
    for (j = 0; j < 4010 * delayMs; j++)
    {
        asm(" NOP");
    }
}

void BSP_Delay100US(INT32 delayUs)
{
    volatile  uint32_t j;
    for (j = 0; j < 400 * delayUs; j++)
    {
        asm(" NOP");
    }
}

void BSP_Delay10US(INT32 delayUs)
{
    volatile  uint32_t j;
    for (j = 0; j < 40 * delayUs; j++)
    {
        asm(" NOP");
    }
}

void BSP_Delay1US(INT32 delayUs)
{
    volatile  uint32_t j;
    for (j = 0; j < 4 * delayUs; j++)
    {
        asm(" NOP");
    }
}

void BSP_Nop(void)
{
    asm(" NOP");
}

void BSP_Reboot(void)
{
//	BSP_EnterCri();
//	BSP_GPIO_CfgUSBCable();
//	printf("system reboot\r\n");
//	//printf here
//	BSP_Delay1S(1);
//	NVIC_SystemReset();//system reset
//	while(1); // wait for reboot
}

void HardFault_Handler(void)
{
//	printf("err: hardfault!\r\n");
//	BSP_Reboot();
}

/* nvic functions */
void BSP_NVIC_Config(UINT32 table_offset)
{
//	NVIC_SetVectorTable(NVIC_VectTab_FLASH, table_offset); // Set the Vector Table base location at 0x08000000  	/*  */
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

void BSP_NVIC_SetUSBLP(UINT8 enable)
{
//	NVIC_InitTypeDef NVIC_InitStructure;
//
//	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	if(enable == 0)
//	{
//		NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
//	}
//	else
//	{
//		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	}
//
//	NVIC_Init(&NVIC_InitStructure);
}

void BSP_NVIC_SetDebugCom(UINT8 enable)
{
//	NVIC_InitTypeDef NVIC_InitStructure;
//
//	NVIC_InitStructure.NVIC_IRQChannel = DEBUG_COM_IRQ;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	if(enable == 0)
//	{
//		NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
//	}
//	else
//	{
//		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	}
//
//	NVIC_Init(&NVIC_InitStructure);
}

void BSP_NVIC_SetALLTIM(UINT8 enable)
{
//	NVIC_InitTypeDef   NVIC_InitStructure;
//	FunctionalState cmd = DISABLE;
//
//	if(enable == 0)
//	{
//		cmd = DISABLE;
//	}
//	else
//	{
//		cmd = ENABLE;
//	}
//
//	/* rf timer */
//	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = cmd;
//	NVIC_Init(&NVIC_InitStructure);
//
//	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = cmd;
//	NVIC_Init(&NVIC_InitStructure);
//
//	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = cmd;
//	NVIC_Init(&NVIC_InitStructure);
//
//	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = cmd;
//	NVIC_Init(&NVIC_InitStructure);
}

/* RCC functions */

void BSP_RCC_SetUSB(UINT8 enable)
{
//	if(enable == 0)
//	{
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE); //	Enable the USB clock
//	}
//	else
//	{
//		/* set usb clock */
//		RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);	// Select USBCLK source
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE); //	Enable the USB clock
//	}
}

void BSP_RCC_SetAllGPIO(UINT8 enable)
{
//	if(enable == 0)
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
//							 	| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD
//								| RCC_APB2Periph_GPIOE, DISABLE);
//	}
//	else
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
//							 	| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD
//								| RCC_APB2Periph_GPIOE, ENABLE);
//	}
}

void BSP_RCC_SetAFIO(UINT8 enable)
{
//	if(enable == 0)
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, DISABLE);
//	}
//	else
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
//	}
}

void BSP_RCC_SetGPIOA(UINT8 enable)
{
//	if(enable == 0)
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, DISABLE);
//	}
//	else
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//	}
}

void BSP_RCC_SetGPIOB(UINT8 enable)
{
//	if(enable == 0)
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, DISABLE);
//	}
//	else
//	{
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	}
}

void BSP_RCC_SetSPI2(UINT8 enable)
{
//	if(enable ==0)
//	{
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, DISABLE);
//	}
//	else
//	{
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
//	}
}

void BSP_RCC_SetDebugCom(UINT8 enable)
{
//	if(enable ==0)
//	{
//		DEBUG_COM_RCC_DISABLE;
//	}
//	else
//	{
//		DEBUG_COM_RCC_ENABLE;
//	}
}

void BSP_RCC_SetALLTIM(UINT8 enable)
{
//	if(enable ==0)
//	{
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, DISABLE);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, DISABLE);
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, DISABLE);
//	}
//	else
//	{
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
//	}
}

/* GPIO functions */

void BSP_GPIO_SetAIN(void)
{
//	/* Step 1: Configures all IOs as AIN to reduce the power consumption. */
//	GPIO_InitTypeDef GPIO_InitStructure;
//
//	/* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) */
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void BSP_GPIO_DisableJTAG(void)
{
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); //Disable JTAG
}

void BSP_GPIO_CfgSPI2(void)
{
//	GPIO_InitTypeDef GPIO_InitStruct;
//
//	/* clk miso mosi */
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//	/* nss */
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStruct);
}

void BSP_GPIO_CfgUSBCable(void)
{
//	GPIO_InitTypeDef GPIO_InitStruct;
//
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//	GPIO_SetBits(GPIOA, GPIO_Pin_12);
//	GPIO_ResetBits(GPIOA, GPIO_Pin_12);
}

void BSP_GPIO_SetUSBCable(int enable)
{
//	if(enable != 0)
//	{
//		GPIO_SetBits(GPIOA, GPIO_Pin_12);
//	}
//	else
//	{
//		GPIO_ResetBits(GPIOA, GPIO_Pin_12);
//	}
}

void BSP_GPIO_CfgDebugCom(void)
{
//	GPIO_InitTypeDef GPIO_InitStruct;
//
//	GPIO_InitStruct.GPIO_Pin = DEBUG_COM_TX_PIN;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_Init(DEBUG_COM_TX_PORT, &GPIO_InitStruct);
//
//	GPIO_InitStruct.GPIO_Pin = DEBUG_COM_RX_PIN;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//	GPIO_Init(DEBUG_COM_RX_PORT, &GPIO_InitStruct);
}

//static int debug_pin_status = 0;
void BSP_GPIO_CfgDebugPin(void)
{
//	GPIO_InitTypeDef GPIO_InitStruct;
//
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
//	GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//	GPIO_ResetBits(GPIOA, GPIO_Pin_15);
//	debug_pin_status = 0;
}



void BSP_GPIO_SetRCI(UINT8 enable)
{
//	GPIO_InitTypeDef GPIO_InitStructure;
//
//	if(enable == 0)
//	{
//		GPIO_InitStructure.GPIO_Pin   = A7106_SCS_PIN | A7106_SCK_PIN | A7106_SDIO_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		GPIO_Init(A7106_SCS_PORT, &GPIO_InitStructure);
//
//		GPIO_InitStructure.GPIO_Pin   = A7106_GIO1_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_Init(A7106_GIO1_PORT, &GPIO_InitStructure);
//
//#ifdef USE_PA
//		GPIO_InitStructure.GPIO_Pin   = A7700_TXSW_PIN | A7700_RXSW_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		//GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//		GPIO_Init(A7700_TXSW_PORT, &GPIO_InitStructure);
//#endif
//
//#ifdef USE_POWER_SWITCH
//		GPIO_InitStructure.GPIO_Pin   = RF_POWER_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		//GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//		GPIO_Init(RF_POWER_PORT, &GPIO_InitStructure);
//#endif
//	}
//	else
//	{
//		GPIO_InitStructure.GPIO_Pin   = A7106_SCS_PIN | A7106_SCK_PIN | A7106_SDIO_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		GPIO_Init(A7106_SCS_PORT, &GPIO_InitStructure);
//
//		GPIO_InitStructure.GPIO_Pin   = A7106_GIO1_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_Init(A7106_GIO1_PORT, &GPIO_InitStructure);
//
//#ifdef USE_PA
//		GPIO_InitStructure.GPIO_Pin   = A7700_TXSW_PIN | A7700_RXSW_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		//GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//		GPIO_Init(A7700_TXSW_PORT, &GPIO_InitStructure);
//#endif
//
//#ifdef USE_POWER_SWITCH
//		GPIO_InitStructure.GPIO_Pin   = RF_POWER_PIN;
//		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
//		//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		//GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//		GPIO_Init(RF_POWER_PORT, &GPIO_InitStructure);
//#endif
//	}
}

void BSP_SPI_CfgSPI2(void)
{
//	SPI_InitTypeDef  SPI_InitStructure;
//
//	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //??SPI???????????:SPI??????????
//	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//??SPI????:????SPI
//	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//??SPI?????:SPI????8????
//	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//???????????????
//	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//?????????????(?????)?????
//	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS?????(NSS??)????(??SSI?)??:??NSS???SSI???
//	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;		//??????????:????????256
//	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//???????MSB???LSB???:?????MSB???
//	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRC???????
//	SPI_Init(SPI2, &SPI_InitStructure);  //??SPI_InitStruct???????????SPIx???
//
//	SPI_Cmd(SPI2, ENABLE);
}

void WD_Feed(void)
{

}

void WD_Init(void)
{

}

void BSP_Config()
{
	//do noting, functions move to function "basic_config()" in file main.c
}
