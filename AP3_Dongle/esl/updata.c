#include "updata.h"
#include "updata0.h"
#include "updata1.h"
#include "g1updata.h"
#include "g2updata.h"
#include "updatabdc.h"
#include "debug.h"
#include "data.h"

INT32 updata_check_para(updata_table_t *table)
{
	INT32 ret = 0;

	pdebug("updata para:\r\n");
	pdebug("master id: 0x%02X-0x%02X-0x%02X-0x%02X\r\n", 
			table->master_id[0], table->master_id[1], table->master_id[2], table->master_id[3]);
	pdebug("power=%d,tx_datarate=%d,rx_datarate=%d,esl_work_duration=%d, idx=%d\r\n",
			table->tx_power, table->tx_datarate, table->rx_datarate, table->esl_work_duration,
			table->id_x_ctrl);
	pdebug("deal_duration=%d,tx_interval=%d,tx_duration=%d,mode=%d,num=%d\r\n", 
			table->deal_duration, table->tx_interval, table->tx_duration, table->mode,
			table->num);

	if(table->tx_duration == 0)
	{
		pdebug("warning: set tx_duration = 750\r\n");
		table->tx_duration = 750;
	}

	return ret;
}

INT32 updata_init_data(UINT16 the_updata_cmd, updata_table_t *table,
						UINT32 the_frame1_addr, INT32 the_frame1_len, 
						UINT32 the_updata_addr, INT32 the_updata_len)
{
	INT32 ret = 0;

	if((the_updata_cmd==0) || (the_updata_addr==0) || (the_updata_len==0))
	{
		ret = -1;
	}

	get_updata_para(the_updata_addr, table->master_id);
	
	updata_check_para(table);

	//init 
	table->frame1_addr = the_frame1_addr;
	table->frame1_len = the_frame1_len;
	table->frame1_offset = 0;
	
	table->updata_addr = the_updata_addr;
	table->updata_len = the_updata_len;

	the_updata_addr += LEN_OF_UPDATA_PARA;
	the_updata_len -= LEN_OF_UPDATA_PARA;
	
	switch(the_updata_cmd)
	{
		case CMD_GROUP1_DATA:
		case CMD_GROUPN_DATA:
			if(table->mode == 0)
			{
				init_data(the_updata_addr,the_updata_len, table);
			}
			else // 1
			{
				m1_init_data(the_updata_addr,the_updata_len, table);
			}
			break;
		case CMD_GROUPN_DATA_G2:
			m1_init_data(the_updata_addr,the_updata_len, table);
			break;
		case CMD_GROUPN_DATA_G1:
			g1_init_data(the_updata_addr,the_updata_len, table);
			break;
		case CMD_GROUP1_DATA_BDC:
			break;
		case CMD_GROUP1_DATA_NEWACK:
		case CMD_GROUPN_DATA_NEWACK:
			m1_init_data(the_updata_addr,the_updata_len, table);
			break;
		default:
			ret = -2;
			break;
	}
	
	return ret;
}

INT32 updata_do_updata(UINT16 the_updata_cmd, updata_table_t *table)
{
	INT32 ret = 0;

	switch(the_updata_cmd)
	{
		case CMD_GROUP1_DATA:
		case CMD_GROUPN_DATA:
			if(table->mode == 0)
			{
				updata_loop(table);
			}
			else // 1
			{
				m1_updata_loop(table);
			}
			break;
		case CMD_GROUPN_DATA_G2:
			g2_updata_loop(table);
			break;
		case CMD_GROUPN_DATA_G1:
			g1_updata_loop(table);
			break;
		case CMD_GROUP1_DATA_BDC:
			bdc_updata_loop(table);
			break;
		case CMD_GROUP1_DATA_NEWACK:
		case CMD_GROUPN_DATA_NEWACK:
			m1_updata_loop(table);
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

INT32 updata_make_ack(UINT16 the_updata_cmd, updata_table_t * table, UINT32 *ack_addr, UINT32 *ack_len)
{
	INT32 ret = 0;

	switch(the_updata_cmd)
	{
		case CMD_GROUP1_DATA:
		case CMD_GROUPN_DATA:
			if(table->mode == 0)
			{
				make_ack(table, ack_addr, ack_len);
			}
			else // 1
			{
				m1_make_ack(table, ack_addr, ack_len, 7);
			}
			break;
		case CMD_GROUPN_DATA_G2:
			m1_make_ack(table, ack_addr, ack_len, 8);
			break;
		case CMD_GROUPN_DATA_G1:
			g1_make_ack(table, ack_addr, ack_len);
			break;
		case CMD_GROUP1_DATA_BDC:
			break;
		case CMD_GROUP1_DATA_NEWACK:
		case CMD_GROUPN_DATA_NEWACK:
			m1_make_new_ack(table, ack_addr, ack_len, 8);
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

