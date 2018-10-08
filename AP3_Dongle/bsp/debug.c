#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "debug.h"
#include "bsp_spi.h"
#include "bsp_uart.h"
#include "bsp.h"
#include "sys_cfg.h"

volatile UINT32 s_debug_level = DEBUG_LEVEL_DFAULT;

#define LOG_SIZE    64

unsigned char debug_buf[LOG_SIZE];
int (*debugWrite)(const uint8_t *buf, uint16_t len) ;


void debug_peripheral_init(void)
{
    em_debug_peripheral type = DEBUG_PERIPHERAL;

    switch (type){
        case DEBUG_SPI:
            bspSpiOpen(4000000);
            debugWrite = bspSpiWrite;
            break;
        case DEBUG_UART:
            my_UART_open();
            debugWrite = UART_send;
            break;
        default:
            break;
    }
}
UINT8 Debug_GetLevel(void)
{
    return s_debug_level;
}

void Debug_SetLevel(UINT8 new_level)
{
    s_debug_level = new_level;
}


#ifdef  XMODELOG
void log_print(const char *fmt, ...)
{
    int len = 0;
    int i = 0;
    uint8_t *ptr = debug_buf;
    memset(debug_buf,0,sizeof(debug_buf));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf((char *)debug_buf, LOG_SIZE - 1, fmt, ap);
    va_end(ap);

    len = strlen((char *)debug_buf);
//    debugWrite(ptr,len);
    for(i=0;i<len;i++)
    {
        debugWrite(ptr++,1);
        BSP_Delay10US(GGGDELAY);

    }
}


void pprint(const char *format, ...)
{
    int len = 0;
    int i = 0;
    uint8_t *ptr = debug_buf;
    memset(debug_buf,0,sizeof(debug_buf));
    va_list ap;
    va_start(ap, format);
    vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
    va_end(ap);

    len = strlen((char *)debug_buf);
    for(i=0;i<len;i++)
    {
        debugWrite(ptr++,1);
        BSP_Delay10US(GGGDELAY);
    }
}

void pdebughex(UINT8 *src, UINT16 len)
{
	INT32 i;

	if(s_debug_level >= DEBUG_LEVEL_DEBUG)
	{
		for(i = 0; i < len; i++)
		{
			if(i != (len-1))
			{
			    pprint("%02X,", src[i]);
			}
			else
			{
			    pprint("%02X.\r\n", src[i]);
			}
		}
	}
}

void perrhex(UINT8 *src, UINT16 len)
{
	INT32 i;

	if(s_debug_level >= DEBUG_LEVEL_ERROR)
	{
		for(i = 0; i < len; i++)
		{
			if(i != (len-1))
			{
			    pprint("%02X,", src[i]);
			}
			else
			{
			    pprint("%02X.\r\n", src[i]);
			}
		}
	}
}

void phex(UINT8 *src, UINT16 len)
{
	INT32 i;

		for(i = 0; i < len; i++)
		{
			if(i != (len-1))
			{
			    pprint("%02X,", src[i]);
			}
			else
			{
			    pprint("%02X\r\n", src[i]);
			}
		}
}

#if 0
void pdebug(const char *format, ...)
{
		va_list args;
		
		printf("dbg: ");	
		va_start(args,format);
		vprintf(format,args);
		va_end(args);		
}

void perr(const char *format, ...)
{
		va_list args;
		
		printf("err: ");
		va_start(args,format);
		vprintf(format,args);
		va_end(args);		
}

void pinfo(const char *format, ...)
{
		va_list args;

		printf("info: ");	
		va_start(args,format);
		vprintf(format,args);
		va_end(args);		
}
#else

void pdebug(const char *format, ...)
{
    int len = 0;
    int i = 0;
    uint8_t *ptr = debug_buf;

	if(s_debug_level >= DEBUG_LEVEL_DEBUG)
	{
	    memset(debug_buf,0,sizeof(debug_buf));
	    va_list ap;
	    va_start(ap, format);
	    vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
	    va_end(ap);

	    len = strlen((char *)debug_buf);
	    for(i=0;i<len;i++)
	    {
	        debugWrite(ptr++,1);
	        BSP_Delay10US(GGGDELAY);
	    }
	}
}

void perr(const char *format, ...)
{
    int len = 0;
    int i = 0;
    uint8_t *ptr = debug_buf;

	if(s_debug_level >= DEBUG_LEVEL_ERROR)
	{
	    memset(debug_buf,0,sizeof(debug_buf));
	    va_list ap;
	    va_start(ap, format);
	    vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
	    va_end(ap);

	    len = strlen((char *)debug_buf);
	    for(i=0;i<len;i++)
	    {
	        debugWrite(ptr++,1);
	        BSP_Delay10US(GGGDELAY);
	    }
	}
}


void pinfo(const char *format, ...)
{
    int len = 0;
    int i = 0;
    uint8_t *ptr = debug_buf;

	if(s_debug_level >= DEBUG_LEVEL_INFO)
	{
	    memset(debug_buf,0,sizeof(debug_buf));
	    va_list ap;
	    va_start(ap, format);
	    vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
	    va_end(ap);

	    len = strlen((char *)debug_buf);
	    for(i=0;i<len;i++)
	    {
	        debugWrite(ptr++,1);
	        BSP_Delay10US(GGGDELAY);
	    }
	}
}

#endif



//    while (1) {
//        spi_write(tx_buf,LOG_SIZE);
//        log_print("spi_write:%02x:%d:%d",1,2,3);
//        //log_print("02%x",1);
//        Task_sleep(100000);
//    }
#else
void pdebughex(UINT8 *src, UINT16 len){
    uint16_t i=0;
    if(s_debug_level >= DEBUG_LEVEL_DEBUG)
    {
        for(i = 0; i < len; i++)
        {
            if(i != (len-1))
            {
                pinfo("%02X,", src[i]);
            }
            else
            {
                pinfo("%02X.\r\n", src[i]);
            }
        }
    }
}
void pdebug(const char *format, ...)
{
    int len = 0;
    uint8_t *ptr = debug_buf;

    if(s_debug_level >= DEBUG_LEVEL_DEBUG)
    {
        memset(debug_buf,0,sizeof(debug_buf));
        va_list ap;
        va_start(ap, format);
        vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
        va_end(ap);

        len = strlen((char *)debug_buf);
        debugWrite(ptr,len);
    }
}
void perr(const char *format, ...){}
void pinfo(const char *format, ...)
{
    int len = 0;
    uint8_t *ptr = debug_buf;

    if(s_debug_level >= DEBUG_LEVEL_INFO)
    {
        memset(debug_buf,0,sizeof(debug_buf));
        va_list ap;
        va_start(ap, format);
        vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
        va_end(ap);

        len = strlen((char *)debug_buf);
        debugWrite(ptr,len);
    }

}
void pinfoEsl(const char *format, ...)
{
    int len = 0;
    uint8_t *ptr = debug_buf;

    if(s_debug_level >= DEBUG_LEVEL_INFO)
    {
        memset(debug_buf,0,sizeof(debug_buf));
        va_list ap;
        va_start(ap, format);
        vsnprintf((char *)debug_buf, LOG_SIZE - 1, format, ap);
        va_end(ap);

        len = strlen((char *)debug_buf);
        debugWrite(ptr,len);
    }

}
void pprint(const char *format, ...){}
void phex(UINT8 *src, UINT16 len){}
void perrhex(UINT8 *src, UINT16 len){}

void log_print(const char *fmt, ...)
{
    int len = 0;
//    BSP_lowGPIO(DEBUG_TEST);

    uint8_t *ptr = debug_buf;
    memset(debug_buf,0,sizeof(debug_buf));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf((char *)debug_buf, LOG_SIZE - 1, fmt, ap);
    va_end(ap);

    len = strlen((char *)debug_buf);
    debugWrite(ptr,len);

//    BSP_highGPIO(DEBUG_TEST);
}

#endif
