#include "cc2640r2_rf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "rftest.h"
#include "timer.h"
#include "bsp.h"
#include "debug.h"
#include "crc16.h"
#include "corefunc.h"
#include "core.h"
#include "flash.h"


#define CLEAR_FREQ ((uint8_t)1)
#define CLEAR_POWER ((uint8_t)1)
//#define BER_DEBUG
#define RF_BER_TIMOUT_20MS   (20000)

static UINT8 actor = 0;
static UINT8 ber_enable = 1;
static UINT8 power_enable = 1;
static UINT8 fc_enable = 1;

static INT32 test_rounds = 200;

static UINT8 tx_id[4] = {0x52,0x56,0x78,0x53};
static UINT16 tx_datarate = 500;
static UINT8 tx_channel = 99;
static UINT8 tx_power = 2;

//static UINT8 rx_id[4] = {0x50,0xb6,0xac,0xee};
static UINT8 rx_id[4] = {0x50,0x09,0x21,0xee};
static UINT16 rx_datarate = 100;
static UINT8 rx_channel = 99;
static UINT8 rx_power = 2;

extern st_calib_value calib;
/*
 ** local setting
 */
#define TEST_DATA_LEN	26
UINT8 rx_buf[TEST_DATA_LEN+2] = {0};
UINT8 tx_buf[TEST_DATA_LEN] = {0};

UINT8 RFC_CalcBgRssi(UINT8 ch, UINT8 initrssi, UINT8 rssithreshold, UINT8 noiserssi, float factor, int times, UINT8 *dutycycle);


static INT32 ber_rx(void)
{
    INT32 ret = 0;
    UINT8 timer = 0;
    INT8   len = 0;
    if((timer=TIM_Open(100, 100, 0, TIMER_ONCE)) == 0)
    {
      ret = -1;
      goto done;
    }
    while(!TIM_CheckTimeout(timer))
    {
        set_power_rate(tx_power,tx_datarate);
        set_frequence(tx_channel);
        if(0 == send_data(tx_id,tx_buf,TEST_DATA_LEN, RF_BER_TIMOUT_20MS))
        {
            continue;
        }

        memset(rx_buf,0xff,TEST_DATA_LEN);
        set_power_rate(tx_power,rx_datarate);
        len = recv_data_for_hb(rx_id, rx_buf, TEST_DATA_LEN, rx_channel, RF_BER_TIMOUT_20MS);
        if(len <= 0)
        {
            continue;
        }

    }
done:
    TIM_Close(timer);
    return ret;
}

float ber = 0.0;
INT32 ber_err_bit = 0;

void set_random_buf(UINT8 *dst, INT32 len)
{
	INT32 i;
	
	dst[0] = 1; // ber ctrl
	
	for(i = 1; i < len; i++)
	{
		dst[i] = rand();
	}
}

INT32 get_err_bit(UINT8 *str1, UINT8 *str2, UINT8 count)
{
	UINT8 i, a;
	INT32 n = 0;
	
	for (i = 0; i < count; i++)
	{
		a = (*(str1+i))^( *(str2+i));
		while(a != 0)
		{
			n++;
			a = a & (a-1);
		}
	}
	
	return n;		
}

UINT8 rssi_f = 0, rssi = 0, rssi_max = 0, rssi_avg = 0 , rssi_min = 0;

void ber_get_rssi(void)
{	
    rssi = get_recPkgRSSI();

#ifdef BER_DEBUG
    log_print("rssi=%d\r\n", rssi);
#endif
	
	if(rssi_f == 0)
	{
		rssi_f = 1;
		rssi_min = rssi;
		rssi_avg = rssi;
		rssi_max = rssi;
	}
	else
	{
		rssi_min = rssi < rssi_min ? rssi : rssi_min;
		rssi_max = rssi > rssi_max ? rssi : rssi_max;
		rssi_avg = (rssi_avg + rssi) / 2;
	}	
}

INT16 fc_f = 0, fc = 0, fc_max = 0, fc_avg = 0, fc_min = 0;

void ber_get_fc(void)
{
//	fc = RFC_GetFC();
#ifdef BER_DEBUG
    log_print("fc=%d\r\n", fc);
#endif	
	if(fc_f == 0)
	{
		fc_f = 1;
		fc_min = fc;
		fc_max = fc;
		fc_avg = fc;
	}
	else
	{
		fc_min = fc < fc_min ? fc : fc_min;
		fc_max = fc > fc_max ? fc : fc_max;
		fc_avg = (fc_avg + fc) / 2;
	}
}

static INT32 make_tx_result(UINT8 *dst, INT32 size)
{
	INT32 ret = 0;
	UINT8 *ptr = dst;
	
	if(size < 16)
	{
		ret = -1;
		goto done;
	}

	if(ber_enable == 1)
	{
		ptr[0] = 1;
		memcpy(ptr+1, &ber, 4);
	}
	
	if(power_enable)
	{
		ptr[5] = 1;
		ptr[6] = rssi_min;
		ptr[7] = rssi_avg;
		ptr[8] = rssi_max;
	}
	
	if(fc_enable)
	{
		ptr[9] = 1;
		memcpy(ptr+10, &fc_min, 2);
		memcpy(ptr+12, &fc_avg, 2);
		memcpy(ptr+14, &fc_max, 2);
	}
	
	ret = 16;
	
	done:
	return ret;
}

static INT32 ber_make_result(UINT8* dst, INT32 size)
{
	UINT16 crc = 0;
	UINT32 result_ber = ber * 10000;

	memset(dst, 0, size);
	dst[0] = 99; // ctrl of ber result
	
	if(ber_enable == 1)
	{
		dst[1] = 1;
		memcpy(&dst[2], &result_ber, sizeof(result_ber));
	}

	if(power_enable)
	{
		dst[6] = 1;
		dst[7] = rssi_min;
		dst[8] = rssi_avg;
		dst[9] = rssi_max;
	}
	
	if(fc_enable)
	{
		dst[10] = 1;
		memcpy(&dst[11], &fc_min, 2);
		memcpy(&dst[13], &fc_avg, 2);
		memcpy(&dst[15], &fc_max, 2);
	}

	crc = CRC16_CaculateStepByStep(0, dst, size-2);
	crc = CRC16_CaculateStepByStep(crc, rx_id, 4);
	
	memcpy(dst+size-2, &crc, 2);
	
	return size;
}

static INT32 ber_check_result_ack(UINT8* ack, INT32 len)
{
	UINT16 crc1 = 0, crc2 = 0;
	INT32 ret = 0;

	memcpy(&crc1, ack+len-2, 2);

	crc2 = CRC16_CaculateStepByStep(0, ack, len-2);
	crc2 = CRC16_CaculateStepByStep(crc2, rx_id, 4);

	if(crc1 != crc2)
	{
		ret = -1;
		goto done;
	}

	if(ack[0] != 100)
	{
		ret = -2;
		goto done;
	}

	ret = 1;

done:
	return ret;
}

static INT32 ber_tx_result(void)
{
    UINT8 result_buf[TEST_DATA_LEN] = {0};
    UINT8 ack_buf[TEST_DATA_LEN] = {0};
    INT32 retry = 5;
    INT32 ret = 0;

    ber_make_result(result_buf, sizeof(result_buf));
    while((retry--) >= 0)
    {
        set_power_rate(tx_power,tx_datarate);
        set_frequence(tx_channel);

        if(TEST_DATA_LEN != send_data(rx_id,result_buf,sizeof(result_buf),RF_BER_TIMOUT_20MS))
        {
            continue;
        }

        set_power_rate(tx_power,rx_datarate);
        if(TEST_DATA_LEN != recv_data_for_hb(tx_id,ack_buf, sizeof(ack_buf),rx_channel, RF_BER_TIMOUT_20MS))
        {
            continue;
        }

        if(ber_check_result_ack(ack_buf, sizeof(ack_buf)) == 1)
        {
            ret = 1;
            break;
        }
    }
    pprint("ber tx result ack: ");
    phex(ack_buf, sizeof(ack_buf));
    return ret;
}

uint8_t rx_time_cont = 0,tx_time_cont= 0;

static INT32 ber_tx(void)
{
    INT32 ret = 0;
    UINT8 crc_error = 0;
    INT32 left_rounds = test_rounds;

    /* reset para */
    rssi_f = 0;
    fc_f = 0;
    ber_err_bit = 0;
    fc = 0; fc_max = 0; fc_avg = 0; fc_min = 0;
    rssi = 0; rssi_max = 0; rssi_avg = 0; rssi_min = 0;
    memset(tx_buf,0x55,TEST_DATA_LEN);
    while((left_rounds--) > 0)
    {
        crc_error = 0;

        set_power_rate(tx_power,tx_datarate);
        set_frequence(tx_channel);

        set_random_buf(tx_buf, TEST_DATA_LEN);
        if(0 == send_data(rx_id,tx_buf,TEST_DATA_LEN,RF_BER_TIMOUT_20MS))
        {
            crc_error = 1;
            goto cal_ber;
        }

        memset(rx_buf,0xff,TEST_DATA_LEN);
        set_power_rate(tx_power,rx_datarate);
        if(0 == recv_data_for_hb(tx_id,rx_buf,TEST_DATA_LEN, rx_channel, RF_BER_TIMOUT_20MS))
        {
            crc_error = 1;
            goto cal_ber;
        }
        //       get rssi and fc
        ber_get_rssi();
        ber_get_fc();
cal_ber:
        if(crc_error == 0)
        {
            ber_err_bit += get_err_bit(rx_buf, tx_buf, TEST_DATA_LEN);
        }
        else
        {
            ber_err_bit += TEST_DATA_LEN * 8;
        }

    }
    ber = (float)ber_err_bit/(float)(test_rounds*TEST_DATA_LEN*8);

    return ret;
}

INT32 rft_check_ber_data(UINT8 *src, INT32 len)
{
#if 1
    actor = src[0];
    ber_enable = src[1];
    power_enable = src[2];
    fc_enable = src[3];

    memcpy(&test_rounds, src+4, 4);

    memcpy(tx_id, src+8, 4);
    memcpy(&tx_datarate, src+12, 2);
    tx_channel = src[14];
    tx_power = src[15];

    memcpy(rx_id, src+16, 4);
    memcpy(&rx_datarate, src+20, 2);
    rx_channel = src[22];
    rx_power = src[23];
#else

    UINT8 ttt[4] = {0x22,0x33,0x44,0x55};
    UINT8 rrr[4] = {0x50,0x07,0x0e,0xee};
    actor = 0;
    ber_enable = 1;
    power_enable = 1;
    fc_enable = 1;

    test_rounds = 200;

    memcpy(tx_id, ttt, 4);
    tx_datarate =100;
    tx_channel = 26;
    tx_power = 2;

    memcpy(rx_id, rrr, 4);
    rx_datarate = 100;

    rx_channel = 32;
    rx_power = 2;


#endif

	return actor;
}

void print_ber_para(void)
{
    log_print("****** BER PARA *******\r\n");
    log_print("actor = %d, ber_e = %d, power_e = %d, fc_e = %d, rounds = %d.\r\n", \
			actor, ber_enable, power_enable, fc_enable, test_rounds);
    log_print("txid = %02X-%02X-%02X-%02X, txdatarate = %d, tx_channel = %d, tx_power = %d\r\n", \
			tx_id[0], tx_id[1], tx_id[2], tx_id[3], tx_datarate, tx_channel, tx_power);
    log_print("rxid = %02X-%02X-%02X-%02X, rxdatarate = %d, rx_channel = %d, rx_power = %d\r\n", \
			rx_id[0], rx_id[1], rx_id[2], rx_id[3], rx_datarate, rx_channel, rx_power);
    log_print("**** BER PARA END *****\r\n");
}

void print_ber_result(void)
{
    log_print("****** BER RESULT *******\r\n");

    log_print("ber = %f\r\n", ber);
    log_print("rssi_min = %d, rssi_avg = %d, rssi_max = %d\r\n", rssi_min, rssi_avg, rssi_max);
    log_print("fc_min = %d, fc_avg = %d, fc_max = %d\r\n", fc_min, fc_avg, fc_max);
	
    log_print("**** BER RESULT END *****\r\n");
}

INT32 rft_ber(UINT8 *ack_buf, INT32 size)
{
    INT32 ret = 0;

    if(actor == 0) // gold board
    {
        ber_tx();
        ret = ber_tx_result();
#ifdef BER_DEBUG
        log_print("ber tx result return %d\r\n", ret);
#endif
        ret = make_tx_result(ack_buf, size);
#ifdef BER_DEBUG
        print_ber_result();
#endif
    }
    else //test board
    {
        ber_rx();
    }


    return ret;
}

void rft_tx_null(core_task_t *task)
{
//    set_power_rate(p->p, DATA_RATE_500K);
    int8_t n = 0;
    st_unmodulated_carrier *p = &task->cmd_buf.unmod_carrier;

    switch(p->p){
    case RF_TX_POWER_L0:
        n = 0;
        break;
    case RF_TX_POWER_L1:
        n = 1;
        break;
    case RF_TX_POWER_L2:
        n = 2;
        break;
    case RF_TX_POWER_L3:
        n = 3;
        break;
    default:
        n = 2;
        break;
    }
    if (p->clear_c == CLEAR_FREQ){
        calib.frequency_offset = 0;
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
    }
    if (p->clear_p == CLEAR_POWER){
        calib.power_offset = 0;
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
        config_power();
    }

    set_power_rate(n, DATA_RATE_500K);
    set_frequence(p->c);
    tx_channel = p->c;
	RF_carrierWave(p->actor==EM_START);

    local_task.ack_len = sizeof(st_unmodulated_carrier_ack);
    local_task.ack_buf.unmod_carrier.c = p->c;
    local_task.ack_buf.unmod_carrier.p = p->p;
    local_task.ack_buf.unmod_carrier.frequency = RF_cmdFs.frequency;
    local_task.ack_buf.unmod_carrier.fractFreq = RF_cmdFs.fractFreq;
    local_task.ack_buf.unmod_carrier.power= RF_cmdPropRadioSetup.txPower;
    local_task.ack_ptr = local_task.ack_buf.buf;

}

INT32 calibrate_freq(core_task_t *task)
{
    st_calibration_freq *tmp = &task->cmd_buf.calib_freq;


    if (TEST_FAILED == task->cmd_buf.calib_freq.result){
        calib.frequency_offset = 0;
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
    }else if (TEST_PASS_SAVE == task->cmd_buf.calib_freq.result){
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
    }else{
//        set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);

        if (tmp->flg == EM_UP){
            calib.frequency_offset += tmp->fract_freq;
        }else{
            calib.frequency_offset -= tmp->fract_freq;
        }
        set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);
        set_frequence(tmp->channel);
        RF_carrierWave(true);
    }
    
    local_task.ack_len = sizeof(st_calibration_freq_ack);
    local_task.ack_buf.freq.frequency = RF_cmdFs.frequency;
    local_task.ack_buf.freq.fractFreq = RF_cmdFs.fractFreq;
    local_task.ack_ptr = local_task.ack_buf.buf;
    return CORE_CMD_ACK;
}

INT32 calibrate_power(core_task_t *task)
{
    int8_t n = 0;


    switch(task->cmd_buf.calib_power.power){
        case RF_TX_POWER_L0:
            n = DBM13_BASE;
            break;
        case RF_TX_POWER_L1:
            n = DBM10_BASE;
            break;
        case RF_TX_POWER_L2:
            n = DBM6_BASE;
            break;
        case RF_TX_POWER_L3:
            n = DBM0_BASE;
            break;
        default:
            n = DBM6_BASE;
            break;
    }

    if (TEST_FAILED == task->cmd_buf.calib_power.result){
        calib.power_offset = 0;
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
        config_power();        
    }else if (TEST_PASS_SAVE == task->cmd_buf.calib_power.result){
        Flash_writeInfo((uint8_t*)&calib, sizeof(calib));
        config_power();
    } else{
        if (EM_UP == task->cmd_buf.calib_power.flg){
            calib.power_offset++;
        }else{
            calib.power_offset--;
        }
        n += calib.power_offset;

        n = n<0 ? 0: n;
        n = n>=ALL_POWER_LEVEL ? ALL_POWER_LEVEL-1: n;
        RF_calib_power(n);
        set_frequence(tx_channel);
        RF_carrierWave(true);
    }


    local_task.ack_len = sizeof(st_calibration_power_ack);
    local_task.ack_buf.power.power = RF_cmdPropRadioSetup.txPower;
    local_task.ack_ptr = local_task.ack_buf.buf;
    return CORE_CMD_ACK;
}

#define SCAN_BG_DEBUG

INT32 rft_scan_bg(UINT8 *src, INT32 srclen, UINT8 *dst, INT32 dstsize)
{
	INT32 ret = 0;
	float factor = (float)src[3]/100;
	int scantimes = 0;

	memcpy(&scantimes, &src[4], sizeof(scantimes));

	enter_txrx();

    dst[1] = RFC_CalcBgRssi(src[0], src[1], src[2], src[8], factor, scantimes, &dst[2]);
	dst[0] = src[0];
	ret = 3;
#ifdef SCAN_BG_DEBUG
	log_print("BG scan channel: %d, rssi = %d, dutycycle = %f\r\n", dst[0], dst[1], (float)dst[2]/100);
#endif

	exit_txrx();
	return ret;
}

#define OFFSET_OF_ACTOR_IN_RFTXRX_CMD		0
#define OFFSET_OF_ID_IN_RFTXRX_CMD			1
#define OFFSET_OF_CHANNEL_IN_RFTXRX_CMD		5
#define OFFSET_OF_BPS_IN_RFTXRX_CMD			6
#define OFFSET_OF_POWER_IN_RFTXRX_CMD		8
#define OFFSET_OF_TIMEOUT_IN_RFTXRX_CMD		9
#define OFFSET_OF_DATALEN_IN_RFTXRX_CMD		13
#define OFFSET_OF_DATA_IN_RFTXRX_CMD		14

INT32 rf_txrx(UINT8 *cmd_buf, INT32 cmd_len, UINT8 *ack_buf, INT32 ack_buf_size)
{
	UINT16 bps = 0;
	UINT8 data_len = *(cmd_buf+OFFSET_OF_DATALEN_IN_RFTXRX_CMD);
	INT32 timeout = 0;
	INT32 ret = 0;
	UINT8 rx_buf[64] = {0};

	//phex(cmd_buf, cmd_len);
	memcpy(&bps, cmd_buf+OFFSET_OF_BPS_IN_RFTXRX_CMD, sizeof(bps));
	memcpy(&timeout, cmd_buf+OFFSET_OF_TIMEOUT_IN_RFTXRX_CMD, sizeof(timeout));
	timeout = timeout*1000;


	if(*(cmd_buf+OFFSET_OF_ACTOR_IN_RFTXRX_CMD) != 0) //tx
	{
//		printf("actor tx %d timeout is %d\r\n", data_len, timeout);
		set_power_rate(*(cmd_buf+OFFSET_OF_POWER_IN_RFTXRX_CMD), bps);
		set_frequence(*(cmd_buf+OFFSET_OF_CHANNEL_IN_RFTXRX_CMD));

		if(send_data(cmd_buf+OFFSET_OF_ID_IN_RFTXRX_CMD,
		             cmd_buf+OFFSET_OF_DATA_IN_RFTXRX_CMD,
		             data_len, timeout)== data_len)
		{
			ret = Core_MakeCmdBuf(CORE_CMD_ACK, NULL, 0, ack_buf, ack_buf_size);
		}
		else
		{
			ret = Core_MakeCmdBuf(CORE_CMD_PARA_ERROR, NULL, 0, ack_buf, ack_buf_size);
		}
	}
	else //rx
	{
	    set_power_rate(RF_DEFAULT_POWER, bps);
        set_frequence(*(cmd_buf+OFFSET_OF_CHANNEL_IN_RFTXRX_CMD));
//		printf("actor rx %d, timeout is %d\r\n", data_len, timeout);
		if(recv_data_for_hb(cmd_buf+OFFSET_OF_ID_IN_RFTXRX_CMD,
		   rx_buf, data_len, *(cmd_buf+OFFSET_OF_CHANNEL_IN_RFTXRX_CMD), timeout)
		   == data_len)
		{
			*(rx_buf+data_len) = get_recPkgRSSI();
			ret = Core_MakeCmdBuf(0x10f0, rx_buf, data_len+1, ack_buf, ack_buf_size);
		}
		else
		{
			ret = Core_MakeCmdBuf(CORE_CMD_PARA_ERROR, NULL, 0, ack_buf, ack_buf_size);
		}
	}
	
	return ret;
}

void RSSI_test(void)
{
    volatile uint8_t readrssi=0;
    uint8_t i = 0;
    while(1)
    {
        RF_measureRSSI(false);
        set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);
        set_frequence(151);
        RF_measureRSSI(true);
        for (i=0; i<200; i++){
            readrssi = RF_readRegRSSI();
            pinfo("%d\r\n", readrssi);
        }
    }

}


UINT8 RFC_CalcBgRssi(UINT8 ch, UINT8 initrssi, UINT8 rssithreshold, UINT8 noiserssi, float factor, int times, UINT8 *dutycycle)
{
    INT32 i = 0, j = 0;
    UINT8 readrssi = 0;
    float frssi = 0.0;
    UINT8 arssi = 0;
    float farssi = 0.0;

    RF_measureRSSI(false);
    set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);
    set_frequence(ch);
    RF_measureRSSI(true);

    if(initrssi != 0)
    {
        frssi = initrssi;
        i = 0;
    }
    else
    {
        readrssi = RF_readRegRSSI();
        frssi = readrssi;
        i = 1;
    }
    for(; i < times; i++)
    {
        readrssi = RF_readRegRSSI();
        frssi = frssi*factor + ((float)readrssi)*(1-factor);
        if(readrssi < rssithreshold)
        {
            j++;
        }

        if(readrssi < noiserssi)
        {
            if(arssi == 0)
            {
                arssi = readrssi;
                farssi = arssi;
            }
            else
            {
                farssi = farssi*factor + ((float)readrssi)*(1-factor);
            }
        }
        BSP_Delay10US(5);
    }

//    RF_idle();

    if(dutycycle != NULL)
    {
        *dutycycle = (UINT8)(((float)j/i)*100);
    }

    if(arssi == 0)
    {
        return (UINT8)frssi;
    }
    else
    {
        return (UINT8)farssi;
    }
}
