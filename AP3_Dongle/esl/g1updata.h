#include "datatype.h"
#include "updata.h"

INT32 g1_init_data(UINT32 addr, INT32 len, updata_table_t *table);
INT32 g1_make_ack(updata_table_t *table, UINT32 *ack_addr, UINT32 *ack_len);
INT32 g1_updata_loop(updata_table_t *table);


