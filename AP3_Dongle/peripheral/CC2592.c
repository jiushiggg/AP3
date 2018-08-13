/*
 * CC2592.C
 *
 *  Created on: 2018Äê2ÔÂ2ÈÕ
 *      Author: ggg
 */
#include <ti/drivers/PIN.h>
#include "CC2640R2_LAUNCHXL.h"
#include "CC2592.h"

static PIN_State cc2592PinState;

const PIN_Config cc2592PinTable[] = {
PA_EN | PIN_INPUT_DIS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /*  initially  */
LNA_EN | PIN_INPUT_DIS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* initially  */
HGM | PIN_INPUT_DIS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* initially  */
PIN_TERMINATE
};

void cc2592Init(void)
{
    PIN_Handle cc2592PinHandle;
    cc2592PinHandle = PIN_open(&cc2592PinState, cc2592PinTable);
    if(!cc2592PinHandle)
    {
        while(1);
    }

}

void cc2592Cfg(paCfg mode)
{
    switch(mode){
        case CC2592_RX_LG_MODE:
            PIN_setOutputValue(&cc2592PinState, PA_EN, 0);
            PIN_setOutputValue(&cc2592PinState, LNA_EN, 1);
            PIN_setOutputValue(&cc2592PinState, HGM, 0);
            break;
        case CC2592_RX_HG_MODE:
            PIN_setOutputValue(&cc2592PinState, PA_EN, 0);
            PIN_setOutputValue(&cc2592PinState, LNA_EN, 1);
            PIN_setOutputValue(&cc2592PinState, HGM, 1);
            break;
        case CC2592_TX:
            PIN_setOutputValue(&cc2592PinState, LNA_EN, 0);
            PIN_setOutputValue(&cc2592PinState, PA_EN, 1);
            PIN_setOutputValue(&cc2592PinState, HGM, 0);
            break;
        case CC2592_POWERDOWN:      //no break;
        default:
            PIN_setOutputValue(&cc2592PinState, PA_EN, 0);
            PIN_setOutputValue(&cc2592PinState, LNA_EN, 0);
            PIN_setOutputValue(&cc2592PinState, HGM, 0);
            break;
    }
}
