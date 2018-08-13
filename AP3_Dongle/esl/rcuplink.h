#ifndef _RCUPLINK_H_
#define _RCUPLINK_H_

#include "datatype.h"

#define RCREQ_BUF_SIZE		1024

#pragma pack(1)
typedef struct
{
	UINT8 id[4];
	UINT8 channel;
	UINT16 recv_bps;
	UINT8 recv_len;
	UINT16 ack_bps;
	UINT8 ack_power;
	UINT8 ack_delay_time;
	UINT32 timeout;
	INT16 recv_pattern;
	UINT8 reserved[5];
	
	UINT16 data_num;
	UINT8 data_buf[RCREQ_BUF_SIZE];
	UINT32 data_len;

	INT32 ack;
	UINT8 rc_ack_len;
	UINT8 rc_ack_buf[RCREQ_BUF_SIZE];

	UINT8 (*uplink)(UINT8 *src, UINT32 len);
	INT32 uplink_ret;
}rcreq_table_t;
#pragma pack()

#define CTRL_OF_RCREQ       0x03 //rc uplink
#define CTRL_OF_STREQ       0x05 //signal test

#define IS_VALID_CTRL(c)		((c == CTRL_OF_RCREQ) \
								|| (c == CTRL_OF_STREQ) )

INT32 RcReq_ParseCmd(UINT8 *pCmd, INT32 cmdLen, rcreq_table_t *table);
INT32 RcReq_Mainloop(rcreq_table_t *table, UINT8 (*uplink)(UINT8 *src, UINT32 len));

#endif


