#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

#include "protocol.h"
//#define GOLD_BOARD
//#define DEBUG_CTRL_OF_OSD

#define MODE1             //Linux <---UART---> Dongle
//#define MODE2             //Linux <---SPI----> Dongle


#if defined(MODE1)
#define PROTOCOL_TYPE   PROTOCOL_XMODEM

#elif defined(MODE2)
#define PROTOCOL_TYPE   PROTOCOL_SPI

#else

#endif

#endif
