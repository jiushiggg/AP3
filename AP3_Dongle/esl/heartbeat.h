#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#include "datatype.h"
#include "updata.h"

#define G3_HB_BUF_SIZE			HB_ESL_BUF_SIZE
#define NUM_OF_MAX_ESL_UPLINK_INFO	10

#pragma pack(1)

typedef struct
{
	UINT8 id[4];
	UINT8 index;
	UINT8 sessionid;
	UINT8 modby;
}esl_uplink_info_t;

typedef struct
{
	UINT8 id[4];
	UINT8 channel;
	UINT16 recv_bps;
	UINT8 recv_len;
	
	UINT32 interval;
	UINT32 timeout;
	UINT32 lenout;
	UINT32 numout;
	UINT32 loop;
	UINT32 apid;
	
	UINT16 num;
	UINT8 data_buf[G3_HB_BUF_SIZE];
	UINT32 data_len;
	UINT8 uplink_buf[100];
	INT32 uplink_data_len;
	esl_uplink_info_t esl_uplink_info[NUM_OF_MAX_ESL_UPLINK_INFO];
}common_recv_table_t;
#pragma pack()

#define g3_hb_table_t common_recv_table_t

UINT8 set_timer(INT32 timeout);
UINT8 check_timer_timeout(UINT8 timer);
void close_timer(UINT8 timer);
void heartbeat_init(void);
INT32 heartbeat_mainloop(UINT8 *pCmd, INT32 cmdLen, g3_hb_table_t *table, UINT8 (*uplink)(UINT8 *src, UINT32 len));
INT32 common_recv(g3_hb_table_t *table, INT32 (*check_fun)(UINT8 *ptr, UINT8 len));
INT32 common_recv_parse_cmd(UINT8 *pCmd, INT32 cmdLen, g3_hb_table_t *table);

#endif
