#ifndef _ESL_H_
#define _ESL_H_

#include "datatype.h"
#include "updata1.h"


#pragma pack(1)
typedef struct{
	UINT32 sid;
	UINT32 data_addr;
	UINT32 data_len;
	UINT32 ack_addr;
	UINT32 ack_len;
	UINT32 version;
	UINT8  buf[UPDATA_BUF_SIZE];
}esl_updata_t;
#pragma pack()

//INT32 esl(esl_updata_t *esl_updata, UINT32 data_addr, UINT32 data_len, UINT32 *ack_addr, UINT32 *ack_len);
INT32 esl_updata(esl_updata_t *updata);

#endif

