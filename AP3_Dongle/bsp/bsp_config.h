#ifndef _BSP_CONFIG_H_
#define _BSP_CONFIG_H_
//#include "stm32f10x.h"

/*
** RF Chip GPIO Config
*/

#define USE_PA
#define USE_POWER_SWITCH

// A7106 and A7700 io define and functions
#define 	A7106_SCS_PORT			GPIOA
#define 	A7106_SCS_PIN			GPIO_Pin_4
#define 	A7106_SCK_PORT			GPIOA
#define 	A7106_SCK_PIN			GPIO_Pin_5
#define 	A7106_SDIO_PORT			GPIOA
#define 	A7106_SDIO_PIN			GPIO_Pin_7
#define 	A7106_GIO1_PORT	 		GPIOA
#define 	A7106_GIO1_PIN	 		GPIO_Pin_6
//#define 	A7106_GIO2_PORT	 	GPIOB
//#define 	A7106_GIO2_PIN	 		GPIO_Pin_0

#ifdef USE_PA
#define 	A7700_TXSW_PORT	 		GPIOB
#define 	A7700_TXSW_PIN	  		GPIO_Pin_1
#define 	A7700_RXSW_PORT	 		GPIOB
#define 	A7700_RXSW_PIN	  		GPIO_Pin_2
#endif

#ifdef USE_POWER_SWITCH
#define 	RF_POWER_PORT	 		GPIOB
#define 	RF_POWER_PIN	  		GPIO_Pin_9
#endif


/*
** SPI2 NSS GPIO Config
*/
#define SPI2_NSS_PORT		GPIOB
#define SPI2_NSS_PIN		GPIO_Pin_12


/*
** COM Config
*/
#define DEBUG_USE_USART1

#ifdef DEBUG_USE_USART1
#define DEBUG_COM						USART1
#define DEBUG_COM_TX_PORT				GPIOA
#define DEBUG_COM_TX_PIN				GPIO_Pin_9
#define DEBUG_COM_RX_PORT				GPIOA
#define DEBUG_COM_RX_PIN				GPIO_Pin_10
#define DEBUG_COM_RCC_ENABLE			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE)
#define DEBUG_COM_RCC_DISABLE			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE)
#define DEBUG_COM_ISR_FUNCTION			USART1_IRQHandler
#define DEBUG_COM_ISR_FUNC				DebugCom_ISR
#define DEBUG_COM_IRQ					USART1_IRQn
#else
#define DEBUG_COM						USART2
#define DEBUG_COM_TX_PORT				GPIOA
#define DEBUG_COM_TX_PIN				GPIO_Pin_2
#define DEBUG_COM_RX_PORT				GPIOA
#define DEBUG_COM_RX_PIN				GPIO_Pin_3
#define DEBUG_COM_RCC_ENABLE			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE)
#define DEBUG_COM_RCC_DISABLE			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, DISABLE)
#define DEBUG_COM_ISR_FUNCTION			USART2_IRQHandler
#define DEBUG_COM_ISR_FUNC				DebugCom_ISR
#define DEBUG_COM_IRQ					USART2_IRQn
#endif





#endif
