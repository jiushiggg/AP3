
#ifndef _DEBUG_H
#define _DEBUG_H
#include "datatype.h"
//#define GGGDEBUG(x)  printf x
//#define X_DEBUG(x) log_print x
#define X_DEBUG(x)    ((void)0)
//#define SPIP_DEBUG(x)    ((void)0)
#define SPIP_DEBUG(x)    log_print x
#define SPIP_DEBUG_REC(x) log_print x //((void)0)
#define SPIP_DEBUG_ERR(x) log_print x
#define GGGDEBUG(x)    ((void)0)
#define DEBUG_LEVEL_HST			0
#define	DEBUG_LEVEL_ERROR		1	
#define	DEBUG_LEVEL_INFO		2	
#define	DEBUG_LEVEL_DEBUG		3

#define DEBUG_LEVEL_DFAULT		DEBUG_LEVEL_DEBUG


typedef enum{
    DEBUG_SPI = (uint8_t)0,
    DEBUG_UART = (uint8_t)1,
}em_debug_peripheral;


UINT8 Debug_GetLevel(void);
void Debug_SetLevel(UINT8 new_level);
void pdebughex(UINT8 *src, UINT16 len);
void pdebug(const char *format, ...);
void perr(const char *format, ...);
void pinfo(const char *format, ...);
void pprint(const char *format, ...);
void phex(UINT8 *src, UINT16 len);
void perrhex(UINT8 *src, UINT16 len);
extern void debug_peripheral_init(void);
void log_print(const char *fmt, ...);
void pinfoEsl(const char *format, ...);
void spidebughex(UINT8 *src, UINT16 len);
#endif
