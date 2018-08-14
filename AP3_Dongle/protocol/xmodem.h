#ifndef _XMODEM_H
#define _XMODEM_H

#include "datatype.h"
#include "protocol.h"
#define XMODEM_LEN_CMD          1
#define XMODEM_LEN_SN           1
#define XMODEM_LEN_DAT          512
#define XMODEM_LEN_CRC          2
#define XMODEM_LEN_ALL          (XMODEM_LEN_CMD+XMODEM_LEN_SN+XMODEM_LEN_DAT+XMODEM_LEN_CRC)


#define XCB_BUF_SIZE   XMODEM_LEN_ALL
extern UINT8 xcb_buf[XCB_BUF_SIZE];
extern INT32 xcb_recv_len;
extern UINT8 recv_once_buf[XMODEM_LEN_ALL];
extern st_protocolFnxTable xmodemFnx;
#endif
