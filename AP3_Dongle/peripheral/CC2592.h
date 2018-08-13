/*
 * CC2592.h
 *
 *  Created on: 2018Äê2ÔÂ2ÈÕ
 *      Author: ggg
 */

#ifndef PERIPHERAL_CC2592_H_
#define PERIPHERAL_CC2592_H_

typedef enum _paCfg{
    CC2592_POWERDOWN = 1,
    CC2592_RX_LG_MODE = 2,
    CC2592_RX_HG_MODE = 4,
    CC2592_TX = 8,
}paCfg;

extern void cc2592Init(void);
extern void cc2592Cfg(paCfg mode);

#endif /* PERIPHERAL_CC2592_H_ */
