#ifndef UPDATA0_H_
#define UPDATA0_H_
#include "datatype.h"
#include "updata.h"

void dummy(updata_table_t *table, INT32 nus);
void dummy_chaining_mode(updata_table_t *table, INT32 nus);
UINT16 init_data(UINT32 addr, UINT32 len, updata_table_t *table);
UINT8 updata_loop(updata_table_t *table);
void make_ack(updata_table_t *table, UINT32 *ack_addr, UINT32 *ack_len);

#endif
