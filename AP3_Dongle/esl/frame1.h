#ifndef _FRAME1_H_
#define _FRAME1_H_

#include "datatype.h"
#include "updata.h"

INT32 frame1_start(UINT16 cmd, UINT32 addr, UINT32 len);
INT32 frame1_dummy(UINT32 addr, UINT32 len, UINT32 *dummy_offset, INT32 dummy_num);

#endif

