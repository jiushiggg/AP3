#ifndef _UPDATA_H_
#define _UPDATA_H_

#include "datatype.h"


#define ESL_NUM 300
#define ESL_BH_NUM  400

#define TABLE_BUF_SIZE  (ESL_NUM*sizeof(mode1_esl_t)+50)
#define UPDATA_BUF_SIZE (TABLE_BUF_SIZE+100)
#define COREMEM_SIZE    (UPDATA_BUF_SIZE+100)


#define HB_ESL_BUF_SIZE (ESL_BH_NUM*27)

#define MAX_FAILED_PKG_NUM  10
#define ESL_REC_FRAME1_TIMEOUT 1200    //1000ms/0.85ms

#pragma pack(1)
//36byte
typedef struct {
    UINT8  esl_id[4];
    UINT8  ack;
    UINT8  sleep_flag;
    UINT32 first_pkg_addr;
    UINT16 total_pkg_num;
    UINT16 failed_pkg_offset;
    UINT16  failed_pkg_num;
    UINT8  failed_pkg[MAX_FAILED_PKG_NUM*2];
} mode1_esl_t;

#define OFFSET_FIRST_PKG_ADDR   7

typedef struct {
	//para area
	UINT8  master_id[4];
	UINT8  tx_power;
	UINT16 tx_datarate;
	UINT16 rx_datarate;
	UINT8  esl_work_duration;
	UINT8  id_x_ctrl;
	UINT8  mode;
	UINT8  deal_duration;
	UINT8  tx_interval;
	UINT16 tx_duration; //us
	UINT8 reserved[5];
	UINT16 num;

	//frame1 area
	UINT32 frame1_addr;
	UINT32 frame1_len;
	//UINT32 frame1_data_addr;
	UINT32 frame1_offset;
	
	UINT32 updata_addr;
	UINT32 updata_len;
	
	UINT16 max_esl_num;
	UINT16 esl_num;
	UINT16 pkg_num;
	UINT16 real_pkg_num;
	UINT16 ok_esl_num;
	UINT8  data[TABLE_BUF_SIZE];

	//no use
	UINT32 sid;
} updata_table_t;
#pragma pack(1)

INT32 updata_init_data(UINT16 the_updata_cmd, updata_table_t *table,
						UINT32 the_frame1_addr, INT32 the_frame1_len, 
						UINT32 the_updata_addr, INT32 the_updata_len);
INT32 updata_do_updata(UINT16 the_updata_cmd, updata_table_t *table);
INT32 updata_make_ack(UINT16 the_updata_cmd, updata_table_t * table, UINT32 *ack_addr, UINT32 *ack_len);
#endif

