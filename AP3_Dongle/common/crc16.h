#ifndef COMMON_CRC16_H_
#define COMMON_CRC16_H_

#include "datatype.h"

UINT16 Crc16_Cal(const UINT8* buf, UINT32 len);
UINT16 CRC16_CaculateStepByStep(UINT16 crc, const UINT8* buf, UINT32 len);

#endif
