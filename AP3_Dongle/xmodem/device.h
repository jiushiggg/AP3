#ifndef _DEVICE_H
#define _DEVICE_H
#include "datatype.h"


void Device_Init(void);
/*
** client: type=1, servie: type=0
*/
extern INT32 Device_Open(UINT8 type, UINT8 *ip, UINT16 port);
extern INT32 Device_Send(INT32 d, UINT8 *src, INT32 len, INT32 timeout);
extern INT32 Device_Recv(INT32 d, UINT8 *dst, INT32 len, INT32 timeout);
extern INT32 Device_Select(INT32 d, INT32 timeout);
extern void Device_Close(INT32 d);

#endif


