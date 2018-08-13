#include "cc2640r2_rf.h"
#include "g2updata.h"
#include "frame1.h"
#include "updata.h"
#include "updata0.h"
#include "updata1.h"
#include "timer.h"
#include "bsp.h"
#include "flash.h"
#include "debug.h"
#include "data.h"
#include "common.h"
#include <string.h>
#include "sleep.h"
#include "sys_cfg.h"
#include "core.h"
#include "ti\drivers\dpl\HwiP.h"

//INT32 search_pkg_sn_times = 0;
//UINT16 search_pkg_history[40] = {0}; 
//UINT32 search_addr_history[40] = {0};
//INT32 search_end_pkg = 0;

//UINT8 search_first_pkg[32] = {0};
//UINT16 search_pkg_history_o[40] = {0};

#define G2_RF_CHANING_MODE
static void g2_transmit(updata_table_t *table, UINT8 timer)
{
    INT32 i = 0, j = 0, k = 0, t = 0;
    UINT8 id[4] = {0};
    UINT8 ch = 0;
    UINT8 len = 0;
    UINT8 data[SIZE_ESL_DATA_BUF] = {0};
    INT32 left_pkg_num = 0;
    INT32 dummy_us = 0;
    mode1_esl_t *pESL = (mode1_esl_t *)table->data;
    UINT16 tsn = 0;
    UINT32 taddr = 0;
    UINT8 f = 0;
    uint16_t result=0;
    UINT8 rf_flg = RF_IDLE;
    uint16_t  key;

#ifdef G2_RF_CHANING_MODE
    send_chaningmode_init();
    write2buf = listInit();
#endif
    set_power_rate(table->tx_power, table->tx_datarate);

    for(i = 0; i < table->esl_num; i++)
    {
        left_pkg_num += pESL[i].failed_pkg_num;
    }
    pdebug("g2_transmit pkg_num %d, timer %d\r\n", left_pkg_num, timer);
    i = 0;
    j = 0;
    while(left_pkg_num > 0)
    {
        if(Core_GetQuitStatus() == 1)
        {
            pdebug("g2_transmit quit\r\n");
            break;
        }

        if(TIM_CheckTimeout(timer) == TIME_OUT)
        {
            pdebug("timeout.\r\n");
            break;
        }

        if((pESL[i].failed_pkg_num != 0) && (j < pESL[i].failed_pkg_num))
        {
            if(pESL[i].failed_pkg_num == pESL[i].total_pkg_num)
            {
                taddr = pESL[i].first_pkg_addr + j*SIZE_ESL_DATA_SINGLE;
                pdebug("send addr 0x%08X\r\n", taddr);
            }
            else
            {
                tsn = get_missed_sn_r(pESL[i].failed_pkg, j);

                //for debug
//              search_pkg_sn_times = 0;
//              memset(search_pkg_history, 0, sizeof(search_pkg_history));
//              memset(search_addr_history, 0, sizeof(search_addr_history));
//              search_end_pkg = 0;
//              memset(search_pkg_history_o, 0, sizeof(search_pkg_history_o));
//              memset(search_first_pkg, 0, sizeof(search_first_pkg));

                taddr = get_pkg_addr_bsearch(pESL[i].first_pkg_addr, pESL[i].total_pkg_num, tsn, 8);
                if(taddr == 0)
                {
                    perr("g2t can't find %02X-%02X-%02X-%02X pkg %d 0x%08X, 0x%08X, %d\r\n",
                        pESL[i].esl_id[0],pESL[i].esl_id[1],pESL[i].esl_id[2],pESL[i].esl_id[3],
                        tsn, taddr, pESL[i].first_pkg_addr, pESL[i].total_pkg_num);
                    //for debug
//                  perr("search end pkg: %d\r\n", search_end_pkg);
//                  perrhex((UINT8 *)search_pkg_history, search_pkg_sn_times*2);
//                  perrhex((UINT8 *)search_addr_history, search_pkg_sn_times*4);
//                  perrhex((UINT8 *)search_pkg_history_o, search_pkg_sn_times*2);
//                  perrhex((UINT8 *)search_first_pkg, 32);

                    left_pkg_num -= pESL[i].failed_pkg_num-j-1;
                    pESL[i].failed_pkg_num = 0;
                    pESL[i].ack = 0x5F;

                    goto user_continue;
                }
//              pESL[i].failed_pkg_offset = (taddr-pESL[i].first_pkg_addr)/SIZE_ESL_DATA_SINGLE + 1;
                pdebug("send miss addr 0x%08X, sn = %d\r\n", taddr, tsn);
            }

            if(get_one_data(taddr, id, &ch, &len, data, SIZE_ESL_DATA_BUF) == 0)
            {
                perr("g2_transmit get data!\r\n");
                goto user_continue;
            }
#ifdef G2_RF_CHANING_MODE
            key = HwiP_disable();
            memcpy(((MyStruct*)write2buf)->tx->pPkt, data ,SIZE_ESL_DATA_BUF);
            ((MyStruct*)write2buf)->tx->syncWord = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
            HwiP_restore(key);
#endif
            pdebug("0x%02X-0x%02X-0x%02X-0x%02X, ch=%d, len=%d\r\n", id[0], id[1], id[2], id[3], ch, len);
            pdebughex(data, len);
#ifdef G2_RF_CHANING_MODE
            if (RF_IDLE == rf_flg){
                rf_flg = RF_WORKING;
                set_frequence(ch);
                memcpy(((MyStruct*)write2buf->next)->tx->pPkt, data ,SIZE_ESL_DATA_BUF);
                result = send_chaningmode(id, data, len, 6000);
                write2buf = List_next(write2buf);
            }else{
                RF_wait_cmd_finish();
            }

#else
            if (PEND_START == rf_flg){
                send_pend(result);
            }
            result = send_without_wait(id, data, len, ch, 6000);
            rf_flg = PEND_START;
#endif
        user_continue:
            left_pkg_num--;
            k++;
            t++;
        }

        f = 0;
        i++;
        if(i == table->esl_num)
        {
        if((dummy_us=((uint16_t)table->tx_interval*1000-k*table->tx_duration))>=0 && rf_flg==RF_WORKING)
            {
#ifdef G2_RF_CHANING_MODE
                dummy_chaining_mode(table, dummy_us);
#else
                dummy(table, dummy_us);
#endif
                f = 1;
            }
            i = k = 0;
            j++;
        }
        if((t%50==0) && (t<ESL_REC_FRAME1_TIMEOUT) && (f==0))
        {
#ifdef G2_RF_CHANING_MODE
            dummy_chaining_mode(table, table->tx_duration);
#else
            dummy(table, table->tx_duration);
#endif
        }
    }
#ifdef G2_RF_CHANING_MODE
    RF_wait_cmd_finish(); //Wait for the last packet to be sent
    RF_cancle(result);
#endif
//    if((i != table->esl_num) && (f == 0))
//    {
//        if((dummy_us=(table->tx_interval*1000-k*table->tx_duration)) >= 0)
//        {
//#ifdef G2_RF_CHANING_MODE
//                dummy_chaining_mode(table, dummy_us);
//#else
//                dummy(table, dummy_us);
//#endif
//            f = 1;
//        }
//        else
//        {
//#ifdef G2_RF_CHANING_MODE
//            dummy_chaining_mode(table, table->tx_duration);
//#else
//            dummy(table, table->tx_duration);
//#endif
//        }
//    }

    wait(2000);
}

static UINT8 query_miss_slot = 0;
static UINT8 first_pkg_data[SIZE_ESL_DATA_BUF] = {0};

static INT32 g2_query_miss(updata_table_t *table, UINT8 timer)
{
	INT32 ret = 0;
	UINT8 data[SIZE_ESL_DATA_BUF] = {0};
	UINT8 rxbuf[SIZE_ESL_DATA_BUF] = {0};
	INT32 i;
	UINT32 deal_timeout = table->deal_duration*1000;
	UINT8 channel = 0;
	mode1_esl_t *pESL = (mode1_esl_t *)table->data;
		
//	set_power_rate(table->tx_power, table->tx_datarate);

	pdebug("g2 query miss(), t=%d, txbps=%d, rxbps=%d, power=%d, timeout=%d.\r\n", timer, \
			table->tx_datarate, table->rx_datarate, table->tx_power, deal_timeout);
	
	for(i = 0; i < table->esl_num; i++, query_miss_slot++)
	{
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("quit\r\n");
			break;		
		}
		
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			pdebug("timeout %d!\r\n", timer);
			break;
		}
		
		if(pESL[i].failed_pkg_num == 0)
		{
			continue;
		}
		
		if(query_miss_slot == 0)
		{
			query_miss_slot = 1;
		}
		set_power_rate(table->tx_power, table->tx_datarate);
		get_one_data(pESL[i].first_pkg_addr, NULL, &channel, NULL, first_pkg_data, sizeof(first_pkg_data));
		pdebug("query 0x%02x-0x%02x-0x%02x-0x%02x, ch=%d.\r\n", \
				pESL[i].esl_id[0], pESL[i].esl_id[1], pESL[i].esl_id[2], pESL[i].esl_id[3], channel);
		memset(data, 0, sizeof(data));
		g2_make_link_query(pESL[i].esl_id, get_pkg_sn_f(pESL[i].first_pkg_addr+(pESL[i].total_pkg_num-1)*32, 8), \
								query_miss_slot, first_pkg_data, data, sizeof(data));
		
		set_frequence(channel);
		send_data(pESL[i].esl_id, data, sizeof(data), 2000);

		set_power_rate(RF_DEFAULT_POWER ,table->rx_datarate);
		set_frequence(channel);
		memset(rxbuf, 0, sizeof(rxbuf));
		if(recv_data(table->master_id, rxbuf, sizeof(rxbuf), deal_timeout) == 0)
		{
			pdebug("recv timeout.\r\n");
			continue;
		}		
		pdebug("recv:");
		pdebughex(rxbuf, sizeof(rxbuf));
		ret++;
		
		if(g2_check_link_query(pESL[i].esl_id, pESL[i].total_pkg_num, query_miss_slot, first_pkg_data, rxbuf, sizeof(rxbuf)) == 0)
		{
			pdebug("data check error.\r\n");
			continue;
		}
	
		memcpy((UINT8 *)&pESL[i].failed_pkg_num, &rxbuf[2], sizeof(pESL[i].failed_pkg_num)); // get failed pkg sn
		pESL[i].failed_pkg_num &= MASK_OF_PKG_SN;
		if(pESL[i].failed_pkg_num == 0) // miss 0 pkg, tx successfully
		{
			//pESL[i].failed_pkg_num = 0;
			pESL[i].ack = rxbuf[4];
#ifdef DEBUG_5
			if(pESL[i].ack != 0x40)
			{
				perr("%02X-%02X-%02X-%02X: %02X\r\n", 
						pESL[i].esl_id[0], pESL[i].esl_id[1], pESL[i].esl_id[2], pESL[i].esl_id[3], pESL[i].ack);
				perr("q: ");
				perrhex(data, sizeof(data));
				perr("r: ");
				perrhex(rxbuf, sizeof(rxbuf));
			}
			else
			{
				pinfo("%02X-%02X-%02X-%02X: 0x40\r\n", 
						pESL[i].esl_id[0], pESL[i].esl_id[1], pESL[i].esl_id[2], pESL[i].esl_id[3]);			
			}
#endif
		}
		else
		{
			if(pESL[i].failed_pkg_num != pESL[i].total_pkg_num)
			{
				pESL[i].failed_pkg_num = pESL[i].failed_pkg_num > MAX_FAILED_PKG_NUM ? MAX_FAILED_PKG_NUM : pESL[i].failed_pkg_num;
				memcpy((UINT8 *)pESL[i].failed_pkg, &rxbuf[4], pESL[i].failed_pkg_num*2);
				pESL[i].failed_pkg_offset = 0;
			}
			pdebug("failed pkg num = %d\r\n", pESL[i].failed_pkg_num);
		}
	}

	dummy(table, table->tx_duration*2);

	wait(2000);
	
	return ret;
}

UINT8 g2_updata_loop(updata_table_t *table)
{
	UINT8 ret = 0;
	UINT8 timer = 0;
	UINT16 timeout = table->esl_work_duration * 10 * 85 / 100;
	UINT16 leftime = 0;
		
	pdebug("g2 updata loop\r\nfirst rount timeout is %d.\r\n", timeout);	
	if((timer=TIM_Open(100, timeout, TIMER_UP_CNT, TIMER_PERIOD)) == TIMER_UNKNOW)
	{
		goto done;
	}
	g2_transmit(table, timer);
	leftime = timeout - TIM_GetCount(timer);
	TIM_Close(timer);
	if(Core_GetQuitStatus() == 1)
	{
		pdebug("quit\r\n");
		goto done;
	}
	timeout = table->esl_work_duration * 10 * 15 / 100 + leftime;
	pdebug("first round left %d ms.\r\nsecond timeout is %d.\r\n", leftime, timeout);
	if((timer=TIM_Open(100, timeout, TIMER_UP_CNT, TIMER_ONCE)) == TIMER_UNKNOW)
	{
		goto done;
	}
	
	while(1)
	{
		g2_query_miss(table, timer);
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			break;
		}
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("quit 1\r\n");
			break;
		}
		m1_send_sleep(table, timer);
		if(table->ok_esl_num == table->esl_num)
		{
			break;
		}
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			break;
		}
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("quit 2\r\n");
			break;
		}		
		g2_transmit(table, timer);
		if(TIM_CheckTimeout(timer) == TIME_OUT)
		{
			break;
		}
		if(Core_GetQuitStatus() == 1)
		{
			pdebug("quit 3\r\n");
			break;
		}		
	}
	
	TIM_Close(timer);

	m1_sleep_all(table);
		
	ret = 1;
	
done:	
	return ret;
}

