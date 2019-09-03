
#include "datatype.h"

#define CMD_SET_WKUP			0x0100
#define CMD_SET_WKUP_TRN		0x0110
#define CMD_SET_WKUP_GLB		0x0120
#define CMD_SET_WKUP_CH			0x0130
#define CMD_SET_WKUP_BDC		0x0101
#define CMD_SET_LED_FLASH       0x0103

#define	CMD_GROUP1				0x0200
#define	CMD_GROUP1_FRAME1		0x0240
#define CMD_GROUP1_FRAME2		0x0241
#define	CMD_GROUP1_DATA			0x0250
#define CMD_GROUP1_DATA_NEWACK	0x0251
#define CMD_GROUP1_DATA_BDC		0x0280
#define CMD_GROUP1_SLEEP		0x02B0

#define	CMD_GROUPN				0x0300
#define	CMD_GROUPN_WKUP			0x0310
#define	CMD_GROUPN_FRAME1		0x0340
#define CMD_GROUPN_DATA			0x0350
#define CMD_GROUPN_DATA_NEWACK	0x0351
#define CMD_GROUPN_DATA_G1		0x0370
#define CMD_GROUPN_DATA_G2		0x0360
#define CMD_GROUPN_SLEEP		0x03B0

#define CMD_ACK					0x0900
#define CMD_ACK_NEW				0x0901

#define OFFSET_FRAME1_PARA		0
#define OFFSET_FRAME1_DATA		13//(2+1+1+1+8)

#define OFFSET_WKUP_PARA		0
#define OFFSET_WKUP_DATA		13//(2+1+1+1+8)

#define OFFSET_DATA_PARA		0x0
#define OFFSET_DATA_DATA		23

#define SIZE_ESL_DATA_PARA		23		
#define SIZE_ESL_DATA_SINGLE	32
#define SIZE_ESL_DATA_BUF		26

#define LEN_OF_UPDATA_PARA		23
#define LEN_OF_FRAME1_PARA		15
#define LEN_OF_BASIC_ESL_DATA	32

//#define FLASH_LED_TEST
#define MASK_OF_PKG_SN			((UINT16)0x3FFF)

#pragma pack(1)
typedef struct st_basic_data_head{
    uint8_t id[4];
    uint8_t ch;
    uint8_t data_len;
}st_basic_data_head;

typedef struct {
	UINT8 id[4];
	UINT8 channel;
	UINT8 len;
	UINT8 data[26];
}g3_seg_data_t;

typedef struct {
	UINT8 id[4];
	UINT8 ack;
}g3_seg_ack_t;

#pragma pack()

INT32 get_updata_para(UINT32 addr, void *dst);
INT32 get_frame1_para(UINT32 addr, void *dst);

INT32 get_one_data(UINT32 addr, UINT8 *id, UINT8 *ch, UINT8 *len, UINT8 *dst, UINT8 size);
UINT16 get_pkg_sn_f(UINT32 pkg_addr, UINT8 sn_offset);
UINT32 get_pkg_addr_bsearch(UINT32 first_pkg_addr, UINT16 pkg_num, UINT16 target_pkg_sn, UINT8 sn_offset);
UINT32 g3_get_sid(UINT32 addr);
UINT8 g3_data_init(UINT32 addr1, UINT8 type1, UINT32 addr2, UINT8 type2);
UINT16 g3_get_cmd(UINT32 addr, UINT32 *cmd_len);
UINT8 g3_get_data(UINT32 addr, UINT8 *id, UINT8 *ch, UINT8 *len, UINT8 *data);
UINT8 g3_get_wkup_para(UINT32 addr, UINT16 *datarate, UINT8 *power, UINT8 *druation, \
						UINT8 *slot_duration, UINT8 *mode);
INT32 g3_get_wkup_loop_times(UINT32 addr);
UINT8 g3_get_frame1_para(UINT32 addr, UINT16 *datarate, UINT8 *power, UINT8 *druation, \
						UINT8 *mode, UINT16 *num);
UINT8 g3_get_update_para(UINT32 addr, UINT8 *master_id, UINT8 *power, UINT16 *tx_datarate, \
						UINT16 *rx_datarate, UINT8 *esl_work_time, UINT8 *idx_ctrl, \
						UINT8 *mode, UINT8 *deal_duration, UINT8 *interval,UINT16 *num);
UINT8 g3_set_ack_para(UINT32 addr, UINT32 sid, UINT16 cmd, UINT32 cmd_len, UINT8 para, UINT16 num);
UINT8 g3_set_ack_crc(UINT32 addr, UINT32 len);
UINT8 g3_set_ack(UINT32 addr, UINT8 *id, UINT8 ack);
UINT8 g3_set_new_ack(UINT32 addr, UINT8 *id, UINT8 ack, UINT8 *detail, UINT8 detail_len);
void g3_print_data(UINT32 addr, UINT32 len);
void g3_print_ack(UINT32 addr, UINT32 len);
void g3_set_print(UINT8 enable);
UINT16 g3_get_wkup_interval(UINT32 wkup_start_addr);
INT32 get_flash_led_data(UINT32 addr, void *data, UINT16 len);
