
#include "datatype.h"

void DebugCom_Conifg(void);
UINT16 DebugCom_SendBuf(const UINT8* src,  UINT16 len);
UINT16 DebugCom_RecvBuf(UINT8 *dst, UINT16 len);
void DebugCom_SetISR(UINT8 enable);
int DebugCom_Select(UINT16 ms);
void DebugCom_ISR(void);
UINT16 DebugCom_GetISRBuf(UINT8 *dst, UINT16 len);
