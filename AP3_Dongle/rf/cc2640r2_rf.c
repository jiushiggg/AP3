#include "cc2640r2_rf.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <xdc/runtime/Error.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include "CC2592.h"
#include "event.h"

#define RF_TEST
#define RF_TX_TEST_IO       IOID_28
#define RF_RX_TEST_IO       IOID_29
#define RF_RX_DATA_TEST_IO  IOID_16
#define RF_RX_SYNC_TEST_IO  IOID_17

#define CRC_ERR             0x40
#define RF_SEND_TIME        900     //900us
/***** Defines *****/
#define myRF_convertMsToRatTicks(microsecond_10)    ((uint32_t)(microsecond_10) * 4 * 10)   // ((uint32_t)(milliseconds) * 4 * 1000)
/* Packet TX/RX Configuration */
#define PAYLOAD_LENGTH      26
#define DATA_ENTRY_HEADER_SIZE 8  /* Constant header size of a Generic Data Entry */
#define MAX_LENGTH             30 /* Max length byte the radio will accept */
#define NUM_DATA_ENTRIES       1  /* NOTE: Only two data entries supported at the moment */
#define NUM_APPENDED_BYTES     2  /* The Data Entries data field will contain:
                                   * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
                                   * Max 30 payload bytes
                                   * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */
#define EASYLINK_RF_EVENT_MASK  ( RF_EventLastCmdDone | RF_EventCmdError | \
             RF_EventCmdAborted | RF_EventCmdStopped | RF_EventCmdCancelled )


#if defined(__TI_COMPILER_VERSION__)
    #pragma DATA_ALIGN (rxDataEntryBuffer, 4);
        static uint8_t rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                                 MAX_LENGTH,
                                                                 NUM_APPENDED_BYTES)];
#elif defined(__IAR_SYSTEMS_ICC__)
    #pragma data_alignment = 4
        static uint8_t rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                                 MAX_LENGTH,
                                                                 NUM_APPENDED_BYTES)];
#elif defined(__GNUC__)
        static uint8_t rxDataEntryBuffer [RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
            MAX_LENGTH, NUM_APPENDED_BYTES)] __attribute__ ((aligned (4)));
#else
    #error This compiler is not supported.
#endif

static void RF_MapIO(void);
void clear_queue_buf(void);
static RF_CmdHandle Rf_rx_package(RF_Handle h,dataQueue_t *dataQueue, uint8_t* id, uint8_t pktLen,uint8_t enableTrigger,  uint32_t  timeout);

rfc_dataEntryGeneral_t* currentDataEntry;
RF_Object rfObject;
RF_Handle rfHandle;
dataQueue_t dataQueue;
RF_Status rf_status = RF_Status_idle;

static UINT8 _hb_rssi = 0;

Semaphore_Handle txDoneSem;
Semaphore_Handle rxDoneSem;

List_List list;
MyStruct foo[2];
List_Elem *write2buf;
UINT8 data0[PAYLOAD_LENGTH] = {0};
UINT8 data1[PAYLOAD_LENGTH] = {0};
st_calib_value calib;

List_Elem* listInit(void)
{
    foo[0].tx =&RF_cmdPropTxAdv[0];
    foo[1].tx =&RF_cmdPropTxAdv[1];

    List_clearList(&list);
    List_put(&list, (List_Elem *)&foo[0]);
    List_put(&list, (List_Elem *)&foo[1]);
    list.tail->next = (List_Elem *)&foo[0];

    return List_head(&list);
}

void txcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if (e & RF_EventCmdAborted)
    {

    }
    if (e & RF_EventCmdDone)
    {
        /* Successful TX */
        write2buf = List_next(write2buf);
        Semaphore_post(txDoneSem);
    }else {

    }
}

void rxcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if(e & RF_EventRxEntryDone)
    {
        Semaphore_post(rxDoneSem);
    }
    if(e & RF_EventLastCmdDone)
    {
          //指令已经执行完成
    }
}

void RF_semaTxPost(void)
{
    Semaphore_post(txDoneSem);
}

void semaphore_RFInit(void)
{
    Semaphore_Params params;
    Error_Block eb;

    /* Init params */
    Semaphore_Params_init(&params);
    Error_init(&eb);
    /* Create semaphore instance */
    rxDoneSem = Semaphore_create(0, &params, &eb);
    params.mode = ti_sysbios_knl_Semaphore_Mode_BINARY;
    txDoneSem = Semaphore_create(0, &params, &eb);
}


void rf_init(void)
{
    RF_Params rfParams;
    RF_Params_init(&rfParams);

    if( RFQueue_defineQueue(&dataQueue, rxDataEntryBuffer,sizeof(rxDataEntryBuffer),NUM_DATA_ENTRIES, MAX_LENGTH + NUM_APPENDED_BYTES))
    {
         /* Failed to allocate space for all data entries */
         while(1);
    }
    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioSetup, &rfParams);
    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

#ifdef  RF_TEST
    RF_MapIO();
#endif
    cc2592Init();
}
#define ESLWORKING_SET
#ifdef ESLWORKING_SET

//baseboard 13dbm, 10dbm, 6dbm, 0dbm
//const uint16_t rf_tx_power[POWER_LEVEL]={0x194e, 0x144b, 0x0cc9, 0x0cc5};
const uint16_t rf_all_tx_power[ALL_POWER_LEVEL]={0x0cc5,0x0cc6, 0x0cc7, 0x0cc9,0x0ccb,0x144b, 0x194e,0x1d52, 0x2558, 0x3161, 0x4214,0x4e18,0x5a1c, 0x9324, 0x9330};
//T3 board 13dbm, 10dbm, 6dbm, 0dbm
uint16_t rf_tx_power[POWER_LEVEL]={0x1d52, 0x194e, 0xCCB, 0x0cc7};
#define POWER_LEVEL2    2
#define POWER_LEVEL_MAX (sizeof(rf_tx_power)/sizeof(rf_tx_power[0])-1)
#else
#define POWER_LEVEL  15
#define MIN_POWER_LEVE -25
#define MAX_POWER_LEVE 5
#define POWER_ZERO_POSITION (POWER_LEVEL - MAX_POWER_LEVE - 1)
#define POWER_DECREASE_VALUE    3
//power greater than 0,  increase by 1. power less than 0, decrease by -3. 0x3161 is 0dbm.0x0cc5 is -25dbm
const uint16_t rf_tx_power[POWER_LEVEL]={0x0cc5,0x0cc6, 0x0cc7, 0x0cc9,0x0ccb,0x144b, 0x194e,0x1d52, 0x2558, 0x3161, 0x4214,0x4e18,0x5a1c, 0x9324, 0x9330};

#endif

const int8_t dbm_base[POWER_LEVEL] = {DBM13_BASE, DBM10_BASE, DBM6_BASE, DBM0_BASE};
void config_power(void)
{
    int8_t n=0, i;
    for (i=0; i<POWER_LEVEL; i++){
        n = dbm_base[i]+calib.power_offset >= ALL_POWER_LEVEL ? ALL_POWER_LEVEL-1 : dbm_base[i]+calib.power_offset;
        n = dbm_base[i]+calib.power_offset < 0 ? DBM6_BASE : dbm_base[i]+calib.power_offset;
        rf_tx_power[i] = rf_all_tx_power[n];
    }
}

void set_frequence(uint8_t  Frequency)
{
    int8_t n = 0;
    RF_cmdFs.frequency = 2400+Frequency/2;
    RF_cmdFs.fractFreq = (Frequency%2 ? 32768 : 0);

    if (calib.frequency_offset == 0){
        goto setfreq;
    }else if (calib.frequency_offset > 0){
        n = (calib.frequency_offset>>16);
        calib.frequency_offset &= 0xffff;
    }else{
        calib.frequency_offset = ~calib.frequency_offset + 1;
        n = 0-(calib.frequency_offset>>16);
        calib.frequency_offset = 0-(calib.frequency_offset&0xffff);
    }

    if (calib.frequency_offset>=32768 && RF_cmdFs.fractFreq==32768){
        RF_cmdFs.fractFreq = calib.frequency_offset - 32768;
        RF_cmdFs.frequency++;
    }else if (calib.frequency_offset>=32768 && RF_cmdFs.fractFreq==0){
        RF_cmdFs.fractFreq = calib.frequency_offset;
    }else if (calib.frequency_offset>=0){
        RF_cmdFs.fractFreq += calib.frequency_offset;
    }else if (calib.frequency_offset<0 && 0==RF_cmdFs.fractFreq){
        RF_cmdFs.fractFreq += 65536+calib.frequency_offset;
        RF_cmdFs.frequency--;
    }else if ((calib.frequency_offset<0 && calib.frequency_offset>=-32768) && 32768==RF_cmdFs.fractFreq){
        RF_cmdFs.fractFreq = RF_cmdFs.fractFreq + calib.frequency_offset;
    }else {  //(calib.frequency_offset>-32768 && (32768==RF_cmdFs.fractFreq || 0==RF_cmdFs.fractFreq){
        RF_cmdFs.fractFreq += 65536+calib.frequency_offset;
        RF_cmdFs.frequency--;
    }
    RF_cmdFs.frequency += n;
setfreq:
    RF_runCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);
}

void RF_calib_power(int8_t Tx_power)
{
    #ifdef ESLWORKING_SET
        RF_cmdPropRadioSetup.txPower = rf_all_tx_power[Tx_power];
    #else
        if (Tx_power<=MAX_POWER_LEVE && Tx_power>=0){
            RF_cmdPropRadioSetup.txPower = rf_tx_power[Tx_power+POWER_ZERO_POSITION];
        } else{
            if (Tx_power<0 && Tx_power>=MIN_POWER_LEVE){
                RF_cmdPropRadioSetup.txPower = rf_tx_power[POWER_ZERO_POSITION - ((~Tx_power+1)+POWER_DECREASE_VALUE-1)/POWER_DECREASE_VALUE];
            }
        }
    #endif
       RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropRadioSetup, RF_PriorityNormal, NULL, 0);
}

void set_power_rate(int8_t Tx_power, uint16_t Data_rate)
{
    switch(Data_rate)
    {
        case DATA_RATE_100K:
            RF_cmdPropRadioSetup.symbolRate.preScale = 15;
            RF_cmdPropRadioSetup.symbolRate.rateWord = 65536;
            RF_cmdPropRadioSetup.modulation.modType = 0x0;
            RF_cmdPropRadioSetup.modulation.deviation = 744;
            RF_cmdPropRadioSetup.rxBw = 9;
			RF_cmdPropRadioSetup.pRegOverride = (uint32_t*)pOverrides100;
        break;
        case  DATA_RATE_500K:
            RF_cmdPropRadioSetup.symbolRate.preScale = 15;
            RF_cmdPropRadioSetup.symbolRate.rateWord = 327680;
            RF_cmdPropRadioSetup.modulation.modType = 0x0;
            RF_cmdPropRadioSetup.modulation.deviation = 744;
            RF_cmdPropRadioSetup.rxBw = 10;
			RF_cmdPropRadioSetup.pRegOverride = (uint32_t*)pOverrides500;
        break;
        case  DATA_RATE_1M:
        break;
        case  DATA_RATE_2M:
        break;
        default:
            RF_cmdPropRadioSetup.symbolRate.preScale = 15;
            RF_cmdPropRadioSetup.symbolRate.rateWord = 327680;
            RF_cmdPropRadioSetup.modulation.modType = 0x0;
            RF_cmdPropRadioSetup.modulation.deviation = 744;
            RF_cmdPropRadioSetup.rxBw = 10;
			RF_cmdPropRadioSetup.pRegOverride = (uint32_t*)pOverrides500;
        break;
    }
#ifdef ESLWORKING_SET
    if (RF_DEFAULT_POWER != Tx_power){
        Tx_power = (Tx_power<0 || Tx_power>POWER_LEVEL_MAX) ? POWER_LEVEL2 : Tx_power;
        RF_cmdPropRadioSetup.txPower = rf_tx_power[Tx_power];
    }

#else
    if (Tx_power<=MAX_POWER_LEVE && Tx_power>=0){
        RF_cmdPropRadioSetup.txPower = rf_tx_power[Tx_power+POWER_ZERO_POSITION];
    } else{
        if (Tx_power<0 && Tx_power>=MIN_POWER_LEVE){
            RF_cmdPropRadioSetup.txPower = rf_tx_power[POWER_ZERO_POSITION - ((~Tx_power+1)+POWER_DECREASE_VALUE-1)/POWER_DECREASE_VALUE];
        }
    }
#endif
   RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropRadioSetup, RF_PriorityNormal, NULL, 0);
}

void send_data_init(UINT8 *id, UINT8 *data, UINT8 len, UINT32 timeout)
{
    RF_cmdPropTxAdv[0].startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropTxAdv[0].startTrigger.pastTrig = 1;
    RF_cmdPropTxAdv[0].startTime = 0;
    RF_cmdPropTxAdv[0].pktLen = len;
    RF_cmdPropTxAdv[0].pPkt = data;
    RF_cmdPropTxAdv[0].syncWord = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
    RF_cmdPropTxAdv[0].pNextOp = NULL;
    /* Only run the RX command if TX is successful */
    RF_cmdPropTxAdv[0].condition.rule = COND_NEVER;
    cc2592Cfg(CC2592_TX);
}
#define MY_TEST_RF
#ifdef MY_TEST_RF
RF_EventMask send_async(uint32_t interal)
{
    RF_EventMask result;
   // RF_cmdPropTxAdv.startTime += interal + EasyLink_us_To_RadioTime(700);
    //result = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv, RF_PriorityNormal, NULL, 0);
    result = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv[0], RF_PriorityNormal, NULL, 0);
//    RF_yield(rfHandle);
    return result;
}



void RF_wait_cmd_finish(void)
{
    Semaphore_pend (txDoneSem, RF_SEND_TIME/Clock_tickPeriod);
}
void send_chaningmode_init(void)
{
    RF_cmdPropTxAdv[0].pPkt = data0;
    RF_cmdPropTxAdv[1].pPkt = data1;
}

uint16_t send_chaningmode(UINT8 *id, UINT8 *data, UINT8 len, UINT32 timeout)
{
    RF_EventMask result;
    cc2592Cfg(CC2592_TX);
    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    RF_cmdPropTxAdv[0].startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropTxAdv[0].startTrigger.pastTrig = 1;
    RF_cmdPropTxAdv[0].startTime = 0;
    RF_cmdPropTxAdv[0].pktLen = len;
    RF_cmdPropTxAdv[0].pPkt = data0;
    RF_cmdPropTxAdv[0].syncWord = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
    RF_cmdPropTxAdv[0].pNextOp = (rfc_radioOp_t *)&RF_cmdPropTxAdv[1];
    /* Only run the RX command if TX is successful */
    RF_cmdPropTxAdv[0].condition.rule = COND_STOP_ON_FALSE;

    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    RF_cmdPropTxAdv[1].startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropTxAdv[1].startTrigger.pastTrig = 1;
    RF_cmdPropTxAdv[1].startTime = 0;
    RF_cmdPropTxAdv[1].pktLen = len;
    RF_cmdPropTxAdv[1].pPkt = data1;
    RF_cmdPropTxAdv[1].syncWord = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
    RF_cmdPropTxAdv[1].pNextOp = (rfc_radioOp_t *)&RF_cmdPropTxAdv[0];
    /* Only run the RX command if TX is successful */
    RF_cmdPropTxAdv[1].condition.rule = COND_STOP_ON_FALSE;

    result = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv[0], RF_PriorityNormal, txcallback,
                        (RF_EventCmdDone | RF_EventLastCmdDone| RF_EventCmdAborted));
    return (uint16_t)result;
}
uint16_t send_flash_led_data(UINT8 *id0,UINT8 *data0, UINT8 *id1, UINT8* data1)
{
    RF_EventMask result;
    cc2592Cfg(CC2592_TX);
    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    RF_cmdPropTxAdv[0].startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropTxAdv[0].startTrigger.pastTrig = 1;
    RF_cmdPropTxAdv[0].startTime = 0;
    RF_cmdPropTxAdv[0].pktLen = 6;
    RF_cmdPropTxAdv[0].pPkt = data0;
    RF_cmdPropTxAdv[0].syncWord = ((uint32_t)id0[0]<<24) | ((uint32_t)id0[1]<<16) | ((uint32_t)id0[2]<<8) | id0[3];
    RF_cmdPropTxAdv[0].pNextOp = (rfc_radioOp_t *)&RF_cmdPropTxAdv[1];
    /* Only run the RX command if TX is successful */
    RF_cmdPropTxAdv[0].condition.rule = COND_STOP_ON_FALSE;

    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    RF_cmdPropTxAdv[1].startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropTxAdv[1].startTrigger.pastTrig = 1;
    RF_cmdPropTxAdv[1].startTime = 0;
    RF_cmdPropTxAdv[1].pktLen = 26;
    RF_cmdPropTxAdv[1].pPkt = data1;
    RF_cmdPropTxAdv[1].syncWord = ((uint32_t)id1[0]<<24) | ((uint32_t)id1[1]<<16) | ((uint32_t)id1[2]<<8) | id1[3];
    RF_cmdPropTxAdv[1].pNextOp = NULL;
    /* Only run the RX command if TX is successful */
    RF_cmdPropTxAdv[1].condition.rule = COND_STOP_ON_FALSE;

    result = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv[0], RF_PriorityNormal, NULL, 0);
    return (uint16_t)result;
}

void send_pend(RF_EventMask result)
{
    //RF_pendCmd(rfHandle, result, RF_EventTxEntryDone|RF_EventLastCmdDone);
}
#else
RF_EventMask send_async(uint32_t interal)
{
    RF_EventMask result;
   // RF_cmdPropTxAdv.startTime += interal + EasyLink_us_To_RadioTime(700);
    result = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv, RF_PriorityNormal, NULL, 0);
    return result;
}

void send_pend(RF_EventMask result)
{
    RF_pendCmd(rfHandle, result, EASYLINK_RF_EVENT_MASK);
}
#endif
void RF_cancle(int16_t result)
{
    RF_cancelCmd(rfHandle, (RF_CmdHandle)result,0);
}
uint8_t send_data(uint8_t *id, uint8_t *data, uint8_t len, uint32_t timeout)
{
    send_data_init(id, data, len, timeout);
    RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTxAdv[0], RF_PriorityNormal, NULL, 0);
    return len;
}
//uint8_t rf_test_buff[26]={0};
UINT8 recv_data(uint8_t *id, uint8_t *data, uint8_t len, uint32_t timeout)
{
    //uint32_t sync_word=0;
    RF_CmdHandle rx_event;
//    set_frequence(ch);

    //sync_word = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
    cc2592Cfg(CC2592_RX_HG_MODE);
    rx_event = Rf_rx_package(rfHandle, &dataQueue, id, len, TRUE , timeout/Clock_tickPeriod);
    if (TRUE ==  Semaphore_pend (rxDoneSem, ((timeout+100)/Clock_tickPeriod))){
        currentDataEntry = RFQueue_getDataEntry();
        memcpy(data, (uint8_t*)(&currentDataEntry->data), len);
        RFQueue_nextEntry();
    }else{
        RF_cancelCmd(rfHandle, rx_event,0);
        currentDataEntry = RFQueue_getDataEntry();
//        memcpy(rf_test_buff, (uint8_t*)(&currentDataEntry->data), len);
        clear_queue_buf();
        len = 0;
    }
    return len;
}
void clear_queue_buf(void)
{
    uint8_t buf[128];
    currentDataEntry = RFQueue_getDataEntry();

    if(DATA_ENTRY_PENDING != currentDataEntry->status)
    {
        memcpy(buf, (uint8_t*)(&currentDataEntry->data), currentDataEntry->length);
        RFQueue_nextEntry();
    }
}

RF_EventMask send_without_wait(UINT8 *id, UINT8 *data, UINT8 len, UINT8 ch, UINT32 timeout)
{
    RF_EventMask result;
    set_frequence(ch);
    send_data_init(id, data, len, timeout);
    result = send_async(timeout);
    return result;
}

static RF_CmdHandle Rf_rx_package(RF_Handle h,dataQueue_t *dataQueue, uint8_t* id, uint8_t pktLen,uint8_t enableTrigger,  uint32_t  timeout)
{
    /* Modify CMD_PROP_RX command for application needs */
    RF_cmdPropRxAdv.syncWord0 = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
    RF_cmdPropRxAdv.pQueue = dataQueue;           /* Set the Data Entity queue for received data */
    RF_cmdPropRxAdv.maxPktLen = pktLen;        /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRxAdv.endTrigger.triggerType = (enableTrigger? TRIG_ABSTIME : TRIG_NEVER );
    RF_cmdPropRxAdv.endTrigger.bEnaCmd = (enableTrigger? 1 : 0 );
    RF_cmdPropRxAdv.endTime = RF_getCurrentTime();
    RF_cmdPropRxAdv.endTime += myRF_convertMsToRatTicks(timeout);
//    RF_cmdPropRxAdv.pktConf.bRepeatOk = 1;
//    RF_cmdPropRxAdv.pktConf.bUseCrc = 0x1;
//    RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropRx, RF_PriorityNormal, &callback, IRQ_RX_ENTRY_DONE);
    RF_CmdHandle result = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropRxAdv, RF_PriorityNormal, &rxcallback, IRQ_RX_ENTRY_DONE);
    return result;
}

void wait(uint32_t nus)
{

}


void enter_txrx(void)
{

}
void exit_txrx(void)
{
//    RF_yield(rfHandle);
    cc2592Cfg(CC2592_POWERDOWN);
}
void RF_idle(void)
{
    RF_yield(rfHandle);
}

void rf_preset_hb_recv(uint8_t b)
{
    if (b){
        RF_cmdPropRxAdv.rxConf.bAutoFlushCrcErr = 0;
        RF_cmdPropRxAdv.pktConf.bRepeatOk = 0x1;
    //    RF_cmdPropRxAdv.pktConf.bUseCrc = 0;
    }else {
        RF_cmdPropRxAdv.rxConf.bAutoFlushCrcErr = 1;
        RF_cmdPropRxAdv.pktConf.bRepeatOk = 0x0;
    //    RF_cmdPropRxAdv.pktConf.bUseCrc = 1;
    }
}


UINT8 recv_data_for_hb(UINT8 *id, UINT8 *data, UINT8 len, UINT8 ch, UINT32 timeout)
{
//    uint32_t sync_word=0;
 //   uint32_t tmp_timeout = 0;
    RF_EventMask rx_event;
    set_frequence(ch);
    cc2592Cfg(CC2592_RX_HG_MODE);
//    sync_word = ((uint32_t)id[0]<<24) | ((uint32_t)id[1]<<16) | ((uint32_t)id[2]<<8) | id[3];
//    tmp_timeout = EasyLink_10us_To_RadioTime(timeout/10);
    rx_event = Rf_rx_package(rfHandle, &dataQueue, id, len, TRUE , timeout/Clock_tickPeriod);
    if (TRUE ==  Semaphore_pend (rxDoneSem, (timeout+100)/Clock_tickPeriod)){
        currentDataEntry = RFQueue_getDataEntry();
        memcpy(data, (uint8_t*)(&currentDataEntry->data), len+2);
        RFQueue_nextEntry();
        RF_cancelCmd(rfHandle, rx_event,0);
        _hb_rssi = data[len];
        if (data[len+1] == CRC_ERR){
            data[len+1] = 0;
            len = 255;
        }
    }else{
        RF_cancelCmd(rfHandle, rx_event,0);
        currentDataEntry = RFQueue_getDataEntry();
//        memcpy(rf_test_buff, (uint8_t*)(&currentDataEntry->data), len);
        clear_queue_buf();
        len = 0;
    }

    return len;
}

//convert CC2640's RSSI to A7106's RSSI. history question ,reference the page 76 of A7106 manual
#define RSSI_FACTOR     31    //(105, 170),(55, 15) => (rssi-15)/(dBm-55) = (170-15)/(105-55) =>rssi = 3.1*dBm-155.5
#define RSSI_CONSTANT   1555

static uint8_t convertRSSI(INT8 n)
{
    uint16_t tmp_rssi = (~n + 1);
    if (tmp_rssi < 50) {
        return 10;
    } else if (tmp_rssi < 55){
        return 15;
    } else {
        return (tmp_rssi*RSSI_FACTOR-RSSI_CONSTANT)/10;
    }
}

uint8_t get_recPkgRSSI(void)
{
    return convertRSSI(_hb_rssi);
}

uint8_t RF_readRegRSSI(void)
{
    int8_t n = RF_getRssi(rfHandle);
    //return ~n+1;
    return convertRSSI(n);
}

void RF_carrierWave(Bool flg)
{
    static RF_CmdHandle cw_ret = RF_Status_idle;
    if (true == flg){
        /* Send CMD_TX_TEST which sends forever */
        cc2592Cfg(CC2592_TX);
        cw_ret = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdTxTest, RF_PriorityNormal, NULL, 0);
        rf_status = RF_Status_carrierWave;
    }else{
        RF_cancle(cw_ret);
        rf_status = RF_Status_idle;
    }


}
void RF_measureRSSI(Bool flg)
{
    static RF_CmdHandle rssi_ret = 0;
    if (true == flg){
        cc2592Cfg(CC2592_RX_HG_MODE);
        rssi_ret = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdRxTest, RF_PriorityNormal, NULL, 0);
        rf_status = RF_Status_measureRSSI;
    }else {
        RF_cancle(rssi_ret);
        rf_status = RF_Status_idle;
    }
}

//untest
void RF_setMeasureRSSI(uint8_t b)
{
    if (b){
        //RF_cmdPropRxAdv.rxConf.bAutoFlushCrcErr = 0;
        RF_cmdPropRxAdv.pktConf.bRepeatOk = 1;
        RF_cmdPropRxAdv.pktConf.bUseCrc = 0;
    }else {
        //RF_cmdPropRxAdv.rxConf.bAutoFlushCrcErr = 1;
        RF_cmdPropRxAdv.pktConf.bRepeatOk = 0;
        RF_cmdPropRxAdv.pktConf.bUseCrc = 1;
    }
}


int16_t set_rx_para(UINT8 *id, UINT16 datarate, UINT8 ch, UINT8 fifosize, UINT32 timeout)
{
    timeout *= 1000000;
    cc2592Cfg(CC2592_RX_HG_MODE);
    set_power_rate(RF_DEFAULT_POWER, datarate);
    set_frequence(ch);
    return Rf_rx_package(rfHandle, &dataQueue, id, fifosize, TRUE , timeout/Clock_tickPeriod);
}

int8_t check_rx_status(UINT16 timeout) //unit ms
{
    timeout = timeout * 1000;

    if (TRUE ==  Semaphore_pend (rxDoneSem, (timeout/Clock_tickPeriod))){
        return 0;
    }else{
        return 1;
    }
}
INT32 get_rx_data(UINT8 *dst, UINT8 dstsize)
{
    currentDataEntry = RFQueue_getDataEntry();
    memcpy(dst, (uint8_t*)(&currentDataEntry->data), dstsize+2);
    RFQueue_nextEntry();
    _hb_rssi = dst[dstsize];
    return 1;
}

static void RF_MapIO(void)
{
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_SYSGPOCTL) = RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0 |RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1 | RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1;
//    IOCIOPortIdSet(RF_RX_SYNC_TEST_IO, IOC_PORT_RFC_GPO3);
//    IOCIOPortIdSet(RF_RX_DATA_TEST_IO, IOC_PORT_RFC_GPO2);
    IOCIOPortIdSet(RF_TX_TEST_IO,      IOC_PORT_RFC_GPO1);
    IOCIOPortIdSet( RF_RX_TEST_IO,      IOC_PORT_RFC_GPO0);
}

#if 0
#define RX_LEN  26
uint8_t mylen =0, mybuf[RX_LEN] = {0};
while(1){
    uint8_t myid[4]={0x52,0x56,0x78,0x53};

    set_power_rate(RF_DEFAULT_POWER, DATA_RATE_100K);
    mylen = recv_data(myid, mybuf, RX_LEN, 2, 1000000);
    if (mylen == 0){
        pinfo("core recv data to flash start.\r\n");
    }
}
#endif
