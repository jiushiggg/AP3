/*
 * communicate.c
 *
 *  Created on: 2018Äê3ÔÂ6ÈÕ
 *      Author: ggg
 */
#include <string.h>
#include <stdint.h>
#include "communicate.h"
#include "uart.h"
#include "xmodem.h"
#include "event.h"
#include "core.h"
#include "debug.h"
#include "bsp.h"
#include "corefunc.h"

void readHandleFnx(void);

void (*tim_soft_callback)(void);


#pragma location = (CMD_BUF_ADDR);
UINT8 cmd_buf[CMD_BUF] = {0};

uint8_t Core_SendCmd(uint16_t cmd, uint32_t cmd_len, uint8_t *cmd_data)
{
    INT32 tx_ack_ret = 0;

    UINT8 ret = 0;
    xmodem_t x;

    if((cmd_len+2+4) > sizeof(cmd_buf))
    {
        perr("Core_SendCmd() cmd len too big.\r\n");
        goto done;
    }

    memcpy(cmd_buf, (void *)&cmd, sizeof(cmd));
    memcpy(cmd_buf+2, (void *)&cmd_len, sizeof(cmd_len));
    if((cmd_len!=0) && (cmd_data!=NULL))
    {
        memcpy(cmd_buf+6, cmd_data, cmd_len);
    }
    memset(&x, 0, sizeof(xmodem_t));
//  USBD_SetRecvMode(0);
    tx_ack_ret = Xmodem_Send(&x, 1, cmd_buf, cmd_len+2+4, 5000);
//  USBD_SetRecvMode(1);
    if(tx_ack_ret == (cmd_len+2+4))
    {
        pdebug("Core_SendCmd 0x%04X\r\n", cmd);
        ret = 1;
    }
    else
    {
        perr("Core_SendCmd 0x%04X return %d\r\n", cmd, tx_ack_ret);
        ret = 0;
    }

done:
    return ret;
}
static uint8_t writeFlashFlg = false;
void communicate_main(void)
{
    uint32_t event = 0;
    while(1){
        event = Event_Pendcommunicate();

        if(event & EVENT_COMMUNICATE_RX_HANDLE){
            readHandleFnx();
            Event_Clear(EVENT_COMMUNICATE_RX_HANDLE);
        }
        if (event & EVENT_COMMUNICATE_RX_TO_FLASH){
            pinfo("cp2flash\r\n");
            memcpy((UINT8 *)&local_task.flash_data_len, local_task.cmd_buf, sizeof(local_task.flash_data_len));
            //BSP_lowGPIO(DEBUG_TEST);
            if(Core_MallocFlash(&local_task.flash_data_addr, local_task.flash_data_len) == 1){
                //BSP_highGPIO(DEBUG_TEST);
                if(Core_SendCmd(CORE_CMD_ACK, 0, NULL) == 1){
                    //BSP_lowGPIO(DEBUG_TEST);
                    writeFlashFlg = true;
                    if(Core_RecvDataToFlash(local_task.flash_data_addr, local_task.flash_data_len) == 1){
                        pinfo(("EVENT_PARSE_DATA\r\n"));
                        Event_Set(EVENT_PARSE_DATA);
                        //BSP_GPIO_test(DEBUG_TEST);
                    }
                    writeFlashFlg = false;
                }
            } else {
                Core_SendCmd(CORE_CMD_FLASH_ERROR, 0, NULL);
            }
            pinfo("cp2flash exit\r\n");
            Event_Clear(EVENT_COMMUNICATE_RX_TO_FLASH);
        }
        if (event & EVENT_COMMUNICATE_TX_ESL_ACK){
            pinfo("core tx esl ack.\r\n");
            Event_Clear(EVENT_COMMUNICATE_TX_ESL_ACK);
        }
        if (event & EVENT_COMMUNICATE_ACK){
            if (NULL != tim_soft_callback){
                tim_soft_callback();
            }
            Event_Clear(EVENT_COMMUNICATE_ACK);
        }

        if(event & EVENT_COMMUNICATE_SCAN_DEVICE){
            pinfo("core uart send ack.\r\n");
            Core_SendCmd(CORE_CMD_ACK, 0, NULL);
            Event_Clear(EVENT_COMMUNICATE_SCAN_DEVICE);
        }

    }
}

void readHandleFnx(void)
{
    int8_t ret = 0;
    ret = Xmodem_RecvCallBack();
    if(ret > CORE_CMD_LEN){
        perr("Xmodem_RecvCallBack recv too big data(%d) to handle.\r\n", ret);
        Xmodem_InitCallback();
    }else if((ret > 0)&&(ret <=XCB_BUF_SIZE)){
        EP_DEBUG(("\r\n>>>EP1_OUT_Callback recv data len = %d.\r\n", ret));
        Core_RxHandler();
        Xmodem_InitCallback();
    }else if(ret < 0){
        EP_DEBUG(("\r\n>>>EP1_OUT_Callback recv error(%d)!\r\n", ret));
        Xmodem_InitCallback();
    }else{
        EP_DEBUG(("\r\n>>>EP1_OUT_Callback.\r\n"));
    }
}

void readCallback(UART_Handle handle, void *rxBuf, size_t size)
{
    if (recCmdAckFlg == true && XMODEM_LEN_CMD==size){
        Device_Recv_post();
    }else if((XMODEM_LEN_CMD==size || XMODEM_LEN_ALL==size) && writeFlashFlg == true){
        Device_Recv_post();
    }else if (XMODEM_LEN_CMD==size || XMODEM_LEN_ALL==size){
        Event_communicateSet(EVENT_COMMUNICATE_RX_HANDLE);
    }else{
        Xmodem_InitCallback();
    }
    xcb_recv_len_once = size;
    UART_appRead(recv_once_buf, XMODEM_LEN_ALL);
}

#ifdef MY_SWI
Void swi0Fxn(UArg arg0, UArg arg1)
{
    //Core_TxHandler();
    tim_soft_callback();
}

void TIM_SetSoftInterrupt(UINT8 enable, void (*p)(void))
{
    tim_soft_callback = p;
    if (1 == enable){
        Swi_post(swi0Handle);
    }

}
void TIM_ClearSoftInterrupt(void)
{

}
#else

void TIM_SetSoftInterrupt(UINT8 enable, void (*p)(void))
{
    tim_soft_callback = p;
    if (1 == enable){
        Event_communicateSet(EVENT_COMMUNICATE_ACK);
    }
}
#endif


