
#ifndef _DEBUG_H
#define _DEBUG_H
#include "datatype.h"
//#define GGGDEBUG(x)  printf x
//#define X_DEBUG(x) log_print x
#define X_DEBUG(x)    ((void)0)
#define GGGDEBUG(x)    ((void)0)
#define DEBUG_LEVEL_HST			0
#define	DEBUG_LEVEL_ERROR		1	
#define	DEBUG_LEVEL_INFO		2	
#define	DEBUG_LEVEL_DEBUG		3

#define DEBUG_LEVEL_DFAULT		DEBUG_LEVEL_DEBUG

UINT8 Debug_GetLevel(void);
void Debug_SetLevel(UINT8 new_level);
void pdebughex(UINT8 *src, UINT16 len);
void pdebug(const char *format, ...);
void perr(const char *format, ...);
void pinfo(const char *format, ...);
void pprint(const char *format, ...);
void phex(UINT8 *src, UINT16 len);
void perrhex(UINT8 *src, UINT16 len);
void debug_peripheral_init(void);
void log_print(const char *fmt, ...);
void pinfoEsl(const char *format, ...);

#endif
