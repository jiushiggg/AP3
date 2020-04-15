#include "timer.h"
#include "debug.h"
#include <ti/drivers/timer/GPTimerCC26XX.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/BIOS.h>
#include "CC2640R2_LAUNCHXL.h"
#include "bsp.h"
#include "event.h"


typedef enum{
    TIMER_IDLE=0,
    TIMER_BUSY=1,
}emTimerStatus;


typedef struct
{
    Clock_Handle TIMn;
    Clock_FuncPtr fnx;
    volatile INT32 count;
    volatile INT32 direction;
    volatile emTimeCheck timeout;
    volatile emTimerStatus inuse;
    volatile emTimerSn sn;
}timer_t;

#define TIRTOS_1MS  (1000/Clock_tickPeriod)

static void ISR_Handle(volatile timer_t* n);
Clock_Handle get_real_timer(UINT8 t);
void SWI_timerCallback0(xdc_UArg n);

Clock_Struct clk0Struct;


//static
volatile timer_t ts[] = {   //todo
	{NULL, SWI_timerCallback0, 0, 0}
//	{NULL, SWI_timerCallback1, 0, 0, 0},
};

static UINT8 get_a_free_timer(void)
{
    INT8 i;
    emTimerSn t = TIMER_UNKNOW;
    UINT8 timer_num = sizeof(ts)/sizeof(timer_t);

    for(i = 0; i < timer_num; i++)
    {
        if(ts[i].inuse == TIMER_IDLE)
        {
            t = ts[i].sn;
            break;
        }
    }

    return t;
}
Clock_Handle get_real_timer(UINT8 t)
{
    if(t <= (sizeof(ts)/sizeof(timer_t)))
    {
        return ts[t].TIMn;
    }
    else
    {
        return NULL;
    }
}

UINT8   getTimerNum(void)
{
    return sizeof(ts)/sizeof(timer_t);
}

INT32 TIM_GetCount(UINT8 t)
{
    if(t <= (sizeof(ts)/sizeof(timer_t)))
    {
        return ts[t].count;
    }
    else
    {
        return -1;
    }
}

UINT32 getticks(void)
{
	return Clock_getTicks();
}

void TIM_Init(void)
{

    Clock_Params clk_Params;

    Clock_Params_init(&clk_Params);
    clk_Params.period = 400000/Clock_tickPeriod;
    clk_Params.startFlag = FALSE;

    /* Construct a periodic Clock Instance */
    Clock_construct(&clk0Struct, (Clock_FuncPtr)SWI_timerCallback0, 400000/Clock_tickPeriod, &clk_Params);
    ts[0].TIMn = Clock_handle(&clk0Struct);
}


UINT8 TIM_Open(UINT32 nms, UINT16 cnt, UINT16 direction, emTimerMode mode)
{
    uint32_t key;
    uint8_t t = 0;
    key = swiDisable();


    if((t=get_a_free_timer()) == TIMER_UNKNOW)
    {
        goto done;
    }
    Clock_stop(ts[t].TIMn);
    ts[t].inuse = TIMER_BUSY; //TODO:



    switch(mode)
    {
        case TIMER_PERIOD:
            if(direction == TIMER_UP_CNT)
            {
                ts[t].count = 0;
                ts[t].direction = cnt;
            }
            else
            {
                ts[t].count = cnt;
                ts[t].direction = 0;
            }
            ts[t].timeout = TIME_COUNTING;
            nms = nms*TIRTOS_1MS;
            Clock_setPeriod(ts[t].TIMn, nms);
            break;
            case TIMER_ONCE:
                if(direction == TIMER_UP_CNT)
                {
                    ts[t].count = 0;
                    ts[t].direction = 1;
                }
                else
                {
                    ts[t].count = 1;
                    ts[t].direction = 0;
                }
                ts[t].timeout = TIME_COUNTING;
                nms = nms*TIRTOS_1MS*cnt;
                Clock_setPeriod(ts[t].TIMn, 0);
                break;
            default:
                break;
    }
    Clock_setTimeout(ts[t].TIMn, nms);
    swiRestore(key);

    Clock_start(ts[t].TIMn);
done:

    return t;
}

void TIM_Close(uint8_t t)
{
    uint32_t key;
    Clock_stop(ts[t].TIMn);
    key = swiDisable();
    ts[t].count = 0;
    ts[t].direction = 0;
    ts[t].timeout = TIME_COUNTING;
    ts[t].inuse = TIMER_IDLE;
    swiRestore(key);
}


UINT8 TIM_CheckTimeout(UINT8 t)
{
	if(get_real_timer(t) != NULL)
	{
//	    pinfoEsl(" %d",ts[t].count);
//		pdebug("timecnt=%d\r\n", ts[t-1].count);
		return ts[t].timeout;
	}
	else
	{
		return 1;
	}
}



void SWI_timerCallback0(xdc_UArg n)
{
    ISR_Handle(&ts[0]);
}
//void SWI_timerCallback1(void)
//{
//    ISR_Handle(&ts[1]);
//}

static void ISR_Handle(volatile timer_t* n)
{
	if(n->TIMn != NULL)
	{
		if(n->direction > 0)
		{
//		    BSP_GPIO_test(DEBUG_TEST);
			if((++n->count) >= n->direction)
			{
				n->timeout = TIME_OUT;
			}
		}
		else
		{
//		    BSP_GPIO_test(DEBUG_TEST);
			if((--n->count) <= 0)
			{
				n->timeout = TIME_OUT;
			}
		}
	}
}
