#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

#include "debug.h"
//#define GOLD_BOARD
//#define DEBUG_CTRL_OF_OSD

//#define   AP_3            //Linux <---UART---> Dongle
#define PCIE             //Linux <---SPI----> Dongle


#if defined(PCIE)
    #define PROTOCOL_TYPE   PROTOCOL_XMODEM
    #define DEBUG_PERIPHERAL    DEBUG_SPI
#elif defined(AP_3)
    #define PROTOCOL_TYPE   PROTOCOL_SPI
    #define DEBUG_PERIPHERAL    DEBUG_UART
#else

#endif

#endif
