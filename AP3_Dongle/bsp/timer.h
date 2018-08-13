#ifndef _TIMER_H_
#define _TIMER_H_

#include "datatype.h"


#define    TIMER_UP_CNT   0
#define    TIMER_DOWN_CNT 1


typedef enum{
    TIME_COUNTING=0,
    TIME_OUT=1,
    TIME_NONE
}emTimeCheck;

typedef enum{
    TIMER0=0,
    TIMER_UNKNOW
}emTimerSn;


typedef enum{
    TIMER_ONCE=0,
    TIMER_PERIOD=1,
    TIMER_UNKNOW_MODE
}emTimerMode;


void TIM_Init(void);
UINT8 TIM_Open(UINT32 nms, UINT16 cnt, UINT16 direction, emTimerMode mode);
void TIM_Close(UINT8 t);
UINT8 TIM_CheckTimeout(UINT8 t);
INT32 TIM_GetCount(UINT8 t);
void TIM_SetSoftTimeout(UINT8 t);
UINT8   getTimerNum(void);
#endif

