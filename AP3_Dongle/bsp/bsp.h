#ifndef _BSP_H
#define _BSP_H

#include "datatype.h"
#include <driverlib/ioc.h>
#include <driverlib/gpio.h>
#include "debug.h"

#define DEBUG_IO0   IOID_16
#define DEBUG_IO1   IOID_17
#define DEBUG_IO2   IOID_18
#define DEBUG_IO3   IOID_19
#define DEBUG_TEST  IOID_15

#if 0
#define LED_ON(n)       GPIO_setDio(n)
#define LED_OFF(n)      GPIO_clearDio(n)
#define LED_TOGGLE(n)   GPIO_toggleDio(n)

#define BSP_DEBUG_IO(n)    BSP_GPIO_test(n)
#else
#define LED_ON(n)
#define LED_OFF(n)
#define LED_TOGGLE(n)

#define BSP_DEBUG_IO(n)

#endif


#if 0
#define EP_DEBUG(x) pprint x
#else
#define EP_DEBUG(x) ((void)0)
#endif

extern void BSP_GPIO_init(void);
extern void BSP_GPIO_test(uint32_t n);
extern void BSP_highGPIO(uint32_t n);
extern void BSP_lowGPIO(uint32_t n);

void BSP_EnterCri(void);
void BSP_ExitCri(void);
void BSP_Delay1S(INT32 n);
void BSP_Delay1MS(INT32 n);
void BSP_Delay100US(INT32 n);
void BSP_Delay10US(INT32 n);
void BSP_Delay1US(INT32 n);
void BSP_Nop(void);
void BSP_Reboot(void);

void BSP_NVIC_Config(UINT32 table_offset);
void BSP_NVIC_SetUSBLP(UINT8 enable);
void BSP_NVIC_SetDebugCom(UINT8 enable);
void BSP_NVIC_SetALLTIM(UINT8 enable);

/* RCC functions */
void BSP_RCC_SetUSB(UINT8 enable);
void BSP_RCC_SetAllGPIO(UINT8 enable);
void BSP_RCC_SetAFIO(UINT8 enable);
void BSP_RCC_SetGPIOA(UINT8 enable);
void BSP_RCC_SetGPIOB(UINT8 enable);
void BSP_RCC_SetSPI2(UINT8 enable);
void BSP_RCC_SetDebugCom(UINT8 enable);
void BSP_RCC_SetALLTIM(UINT8 enable);

/* GPIO Config functions */
void BSP_GPIO_SetAIN(void);
void BSP_GPIO_DisableJTAG(void);
void BSP_GPIO_CfgSPI2(void);
void BSP_GPIO_CfgDebugCom(void);
void BSP_GPIO_SetRCI(UINT8 enable);
void BSP_GPIO_CfgUSBCable(void);
void BSP_GPIO_SetUSBCable(int enable);

void BSP_GPIO_CfgDebugPin(void);
extern void BSP_GPIO_ToggleDebugPin(void);

void BSP_SPI_CfgSPI2(void);

/* watchdog feed */
void WD_Feed(void);
void WD_Init(void);

/* bsp config */
void BSP_Config(void);

#endif
