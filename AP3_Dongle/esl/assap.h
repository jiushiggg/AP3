#ifndef _ASSAP_H_
#define _ASSAP_H_

#include "datatype.h"
#include "heartbeat.h"

#define assap_ack_table_t common_recv_table_t

INT32 assap_scanwkup_parse_cmd(UINT8 *cmd, INT32 cmd_len);
INT32 assap_scanwkup_ret_size(void);
INT32 assap_scan_wkup(UINT8 *dst, INT32 dsize);
INT32 assap_ack_parse_cmd(UINT8 *pCmd, INT32 cmdLen, assap_ack_table_t *table);
INT32 assap_ack(assap_ack_table_t *table);

#endif
