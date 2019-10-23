#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include "event.h"
#include "watchdog.h"

#define EVENT_PEND_TIME ((WD_RESET_TIME_S/4+1)*1000000/Clock_tickPeriod)

Event_Handle protocol_eventHandle;
Event_Struct protocol_eventStruct;
#if defined(TASK1)
Event_Handle communicateEventHandle;
Event_Struct communicateEventStruct;
#endif

Semaphore_Struct  recSemStruct;
Semaphore_Handle  recSemHandle;


void Semphore_xmodemInit(void)
{
    Semaphore_Params  recSemParam;
    Semaphore_Params_init(&recSemParam);
    recSemParam.mode = ti_sysbios_knl_Semaphore_Mode_BINARY;
    Semaphore_construct(&recSemStruct, 0, &recSemParam);
    recSemHandle = Semaphore_handle(&recSemStruct);
}
Bool Device_Recv_pend(UINT32 timeout)
{
    return Semaphore_pend(recSemHandle, timeout);
}
void Device_Recv_post(void)
{
    Semaphore_post(recSemHandle);
}



void Event_init(void)
{
    Event_Params eventParams;
    Event_Params_init(&eventParams);

    Event_construct(&protocol_eventStruct, &eventParams);
    protocol_eventHandle = Event_handle(&protocol_eventStruct);

#if defined(TASK1)
    Event_construct(&communicateEventStruct, &eventParams);
    communicateEventHandle = Event_handle(&communicateEventStruct);
#endif
}

UINT32 Event_Get(void)
{
    return Event_getPostedEvents(protocol_eventHandle);
}


void Event_Set(UINT32 event)
{
    Event_post(protocol_eventHandle, event);
}

void Event_communicateSet(UINT32 event)
{
    Event_post(protocol_eventHandle, event);
}

void Event_Clear(UINT32 event)
{
//    event_flag ^= event;
}
void Event_communicateClear(UINT32 event)
{

}

UINT32 Event_GetStatus(void)
{
    return Event_getPostedEvents(protocol_eventHandle);
}

UINT32 Event_communicateGetStatus(void)
{
    return Event_getPostedEvents(protocol_eventHandle);
}

UINT32 Event_PendCore(void)
{
    return Event_pend(protocol_eventHandle, 0, EVENT_ALL, EVENT_PEND_TIME);

}



uint32_t taskDisable(void)
{
    return Task_disable();
}

void taskRestore(uint32_t key)
{
    Task_restore(key);
}

uint32_t swiDisable(void)
{
    return Swi_disable();
}

void swiRestore(uint32_t key)
{
    Swi_restore(key);
}
