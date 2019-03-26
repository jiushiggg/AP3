/*
 * main.c
 *
 *  Created on: 2018Äê2ÔÂ6ÈÕ
 *      Author: ggg
 */
#include <stdint.h>
#include <stddef.h>

// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SDSPI.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>
#include <ti/drivers/Power.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>

#include <ti/drivers/Power/PowerCC26XX.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/utils/list.h>

/* Board Header file */
#include "Board.h"
#include "cc2640r2_rf.h"
#include "datatype.h"
#include "event.h"
#include "debug.h"
#include "flash.h"
#include "core.h"
//#include "protocol.h"
#include "bsp.h"
#include "bsp_spi.h"
#include "timer.h"
#include "rftest.h"


#pragma location =(0x60)
#ifdef GOLD_BOARD
const unsigned char APP_VERSION_STRING[] = "rfd-6.0.0"; //must < 32
#else
const unsigned char APP_VERSION_STRING[24] = "rfd-6.0.6"; //must < 32
#endif


void *mainThread(void *arg0);
void *communicate2master(void *arg0);

#pragma location = (GPRAM_BASE);
Char task0_Stack[TASK0_STACKSIZE];
Task_Struct task0_Struct;

#if defined(MY_SWI)
    Swi_Handle swi0Handle;
    Swi_Struct swi0Struct;
#elif defined(TASK1)
    #pragma location = (TASK1_ADDR);
    Char task1_Stack[TASK1_STACKSIZE];
    Task_Struct task1_Struct;
#else

#endif

void app_init(void)
{
    Task_Params taskParams_0;

#ifdef MY_SWI
    Swi_Params swiParams;
#endif
    Power_setConstraint(PowerCC26XX_SB_VIMS_CACHE_RETAIN);
    Power_setConstraint(PowerCC26XX_NEED_FLASH_IN_IDLE);

    Task_Params_init(&taskParams_0);
    taskParams_0.arg0 = 1000000 / Clock_tickPeriod;
    taskParams_0.stackSize = TASK0_STACKSIZE;
    taskParams_0.stack = &task0_Stack;
    taskParams_0.priority = 2;
    Task_construct(&task0_Struct, (Task_FuncPtr)mainThread, &taskParams_0, NULL);


#if defined(MY_SWI)
    Swi_Params_init(&swiParams);
    swiParams.arg0 = 0;
    swiParams.arg1 = 0;
    swiParams.priority = 0;
    swiParams.trigger = 0;
    Swi_construct(&swi0Struct, (Swi_FuncPtr)swi0Fxn, &swiParams, NULL);
    swi0Handle = Swi_handle(&swi0Struct);
#elif defined(TASK1)
    Task_Params_init(&taskParams_0);
    taskParams_0.arg0 = 1000000 / Clock_tickPeriod;
    taskParams_0.stackSize = TASK1_STACKSIZE;
    taskParams_0.stack = &task1_Stack;
    taskParams_0.priority = 1;
    Task_construct(&task1_Struct, (Task_FuncPtr)communicate2master, &taskParams_0, NULL);
#else

#endif

    Event_init();
    Semphore_xmodemInit();
}


void *mainThread(void *arg0)
{
    Board_initSPI();
    Board_initUART();
    Board_initGPIO();
    Debug_SetLevel(DEBUG_LEVEL_INFO);

    debug_peripheral_init();
    pinfo("basic init complete.\r\n");

//        while (1) {
//            log_print("spi_write:%02x:%02d:%02d",1,2,3);
//            //log_print("ab%x",1);
//            Task_sleep(10);
//        }

    Core_Init();
    pinfo("core init complete.\r\n");

    if(Flash_Init() == 1)
    {
    	local_task.ack_buf.buf[0] = 0;
        pinfo("flash init successfully.\r\n");
    }
    else
    {
    	local_task.ack_buf.buf[0] = 1;
        perr("flash init fail.\r\n");
    }

    protocol_peripheralInit();
    pinfo("peripheral init complete\r\n");

    pinfo("enter main loop.\r\n");
//#define LIST
#ifdef LIST
    while(1){
        typedef struct MyStruct {
            List_Elem elem;
            uint8_t buffer[26];
        } MyStruct;

        List_List list;
        MyStruct foo[2];

        List_clearList(&list);
        List_put(&list, (List_Elem *)&foo[0]);
        List_put(&list, (List_Elem *)&foo[1]);
        list.tail->next = (List_Elem *)&foo[0];
        //foo[1].elem.next = (List_Elem *)&foo[0];
//        bar = (MyStruct *)List_get(&list);        //delete one element

//        List_List list;
        List_Elem *temp;
        for (temp = List_head(&list); temp != NULL; temp = List_next(temp)) {
           printf("address = 0x%x\r\n", temp);
        }
    }
#endif
//#define GGG_RSSI_TEST
#ifdef GGG_RSSI_TEST
    //
    while(1){
        st_unmodulated_carrier p;
#define CW
#ifdef CW
        p.p = 0;
        p.c = 168;
        p.actor = EM_START;
        memcpy((uint8_t*)&local_task.cmd_buf.unmod_carrier, (uint8_t*)&p, sizeof(st_unmodulated_carrier));
        rft_tx_null(&local_task);
        while(1);
#else
        RSSI_test();
#endif
    }
#endif
//#define GGG_RF_SEND_REC
#ifdef GGG_RF_SEND_REC
#define TEST_CHANNEL    2
    uint8_t i;
    set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);
    set_frequence(TEST_CHANNEL);
    for (i=0; i<26; i++)
        mybuf[i] = i;
    while(1){
        uint8_t id[4] = {0x52,0x56,0x78,0x53};

        set_power_rate(RF_DEFAULT_POWER, DATA_RATE_500K);
        set_frequence(TEST_CHANNEL);
        send_data(id, mybuf, 26, 2000);
//        exit_txrx();
//        set_power_rate(RF_DEFAULT_POWER,DATA_RATE_100K);
//        set_frequence(TEST_CHANNEL);
//        memset(mybuf, 0, sizeof(mybuf));
//        if(recv_data(id, mybuf, sizeof(mybuf), 2000000) == 0)
//        {
//            pdebug("recv timeout.\r\n");
//            continue;
//        }
    }
#endif
#ifdef GGG_CLOCK_TIMER
    while(1){
        uint8_t t=1;

        t = TIM_Open(20, 20000, TIMER_UP_CNT, TIMER_PERIOD);
        while(TIME_COUNTING==TIM_CheckTimeout(t));
        TIM_Close(t);

        t = TIM_Open(20, 200, TIMER_DOWN_CNT, TIMER_PERIOD);
        while(TIME_COUNTING==TIM_CheckTimeout(t));
        TIM_Close(t);


        t = TIM_Open(100, 40, TIMER_UP_CNT, TIMER_ONCE);
        while(TIME_COUNTING==TIM_CheckTimeout(t));

        TIM_Close(t);
    }
#endif
    Core_Mainloop();

    return 0;
}
#if defined(TASK1)
void *communicate2master(void *arg0)
{
    communicate_main();
    return 0;
}
#endif
