#ifndef _EVENT_H_
#define _EVENT_H_

#include "datatype.h"
#include <ti/sysbios/knl/Clock.h>

#define EVENT_NONE				0
#define EVENT_PARSE_DATA		((UINT32)1<<0) //1
#define EVENT_ESL_UPDATA		((UINT32)1<<1) //2
#define EVENT_FW_UPDATA			((UINT32)1<<2) //4
#define EVENT_G3_HEARTBEAT		((UINT32)1<<6) //64
#define EVENT_RC_REQ			((UINT32)1<<7) //64
#define EVENT_SCAN_WKUP			(1<<11)
#define EVENT_ASS_ACK			(1<<12)
#define EVENT_CALIBRATE_POWER   (1<<15)
#define EVENT_CALIBRATE_FREQ    (1<<16)
#define EVENT_RF_TXRX			(1<<17)
#define EVENT_SCAN_BG			(1<<18)
#define EVENT_FT_BER			(1<<19)
#define EVENT_SYSTEM_REBOOT		(1<<20) //64

#if defined(TASK1)
#define EVENT_COMMUNICATE_ACK               ((UINT32)1<<1) //8
#define EVENT_COMMUNICATE_RX_HANDLE         ((UINT32)1<<2) //8
#define EVENT_COMMUNICATE_RX_TO_FLASH       ((UINT32)1<<3) //8
#define EVENT_COMMUNICATE_TX_FROM_FLASH     ((UINT32)1<<4) //A
#define EVENT_COMMUNICATE_TX_ESL_ACK        ((UINT32)1<<5) //
#define EVENT_COMMUNICATE_SCAN_DEVICE        ((UINT32)1<<6) // new cmd
#define EVENT_COMMUNICATE_ALL                              0xFFFFFFFF
#else
#define EVENT_COMMUNICATE_RX_HANDLE         ((UINT32)1<<22) //8
#define EVENT_COMMUNICATE_RX_TO_FLASH       ((UINT32)1<<23) //8
#define EVENT_COMMUNICATE_TX_FROM_FLASH     ((UINT32)1<<24) //A
#define EVENT_COMMUNICATE_SCAN_DEVICE        ((UINT32)1<<26) // new cmd
#endif

#define EVENT_FLASH_ERR			(1<<29)
#define EVENT_RF_ERR			(1<<30)
#define EVENT_ALL   0xFFFFFFFF



#define EVENT_WAIT_FOREVER   ti_sysbios_BIOS_WAIT_FOREVER

#define EVENT_WAIT_US(n)     ((uint32_t)n/Clock_tickPeriod)


typedef enum _eventStatus{
    EVENT_BUSY,
    EVENT_IDLE,
}eventStatus;

extern void Event_init(void);

extern UINT32 Event_Get(void);
extern void Event_Set(UINT32 event);
extern void Event_Clear(UINT32 event);
extern UINT32 Event_GetStatus(void);
extern UINT32 Event_PendCore(void);

extern UINT32 Event_communicateGet(void);
extern void Event_communicateSet(UINT32 event);
extern void Event_communicateClear(UINT32 event);
extern UINT32 Event_communicateGetStatus(void);
extern UINT32 Event_Pendcommunicate(void);

extern void Semphore_xmodemInit(void);
extern Bool Device_Recv_pend(UINT32 timeout);
extern void Device_Recv_post(void);
extern uint32_t taskDisable(void);
extern void taskEnable(uint32_t key);
extern uint32_t swiDisable(void);
extern void swiRestore(uint32_t key);

#endif
