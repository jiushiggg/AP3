#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include "datatype.h"
#include "cc2640r2_rf.h"
#include "core.h"

extern uint8_t rx_time_cont,tx_time_cont;
extern uint8_t ber_timeout_flag;
extern INT32 rft_check_ber_data(UINT8 *src, INT32 len);
extern INT32 rft_ber(UINT8 *ack_buf, INT32 size);
extern INT32 rft_scan_bg(UINT8 *src, INT32 srclen, UINT8 *dst, INT32 dstsize);
extern INT32 rf_txrx(UINT8 *cmd_buf, INT32 cmd_len, UINT8 *ack_buf, INT32 ack_buf_size);
extern void rft_tx_null(core_task_t *task);
extern void RSSI_test(void);
extern INT32 calibrate_freq(core_task_t *task);
extern INT32 calibrate_power(core_task_t *task);
#endif
