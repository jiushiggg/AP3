#ifndef _XMODEM_H
#define _XMODEM_H

#include "datatype.h"

#define XMODEM_LEN_CMD          1
#define XMODEM_LEN_SN           1
#define XMODEM_LEN_DAT          512
#define XMODEM_LEN_CRC          2
#define XMODEM_LEN_ALL          (XMODEM_LEN_CMD+XMODEM_LEN_SN+XMODEM_LEN_DAT+XMODEM_LEN_CRC)

typedef struct {
	UINT8 last_recv_cmd;
	UINT8 last_recv_sn;
	UINT8 send_sn;
	INT32 send_retry_times;
	INT32 nak_times;
}xmodem_t;

void Xmodem_Reset(xmodem_t *x);
INT32 Xmodem_Send(xmodem_t *x, INT32 dev, UINT8 *src, INT32 len, INT32 timeout);
INT32 Xmodem_Recv(xmodem_t *x, INT32 dev, UINT8 *dst, INT32 len, INT32 timeout);
INT32 Xmodem_RecvOnce(xmodem_t *x, INT32 dev, UINT8 **dst, INT32 timeout);
void Xmodem_InitCallback(void);
INT32 Xmodem_RecvCallBack(void);
UINT8 *Xmodem_GetCallbackData(UINT32 *len);
INT32 Xmodem_RecvToFlash(xmodem_t *x, INT32 dev, UINT32 addr, INT32 dst_len, INT32 timeout);
INT32 Xmodem_SendFromFlash(xmodem_t *x, INT32 dev, UINT32 addr, INT32 len, INT32 timeout);


#define XCB_BUF_SIZE   XMODEM_LEN_ALL
extern UINT8 xcb_buf[XCB_BUF_SIZE];
extern INT32 xcb_recv_len;
extern UINT8 recv_once_buf[XMODEM_LEN_ALL];
#endif
