#ifndef _XMODEM_H
#define _XMODEM_H

#include "datatype.h"

#define XMODEM_LEN_CMD          1
#define XMODEM_LEN_SN           1
#define XMODEM_LEN_DAT          512
#define XMODEM_LEN_CRC          2
#define XMODEM_LEN_ALL          (XMODEM_LEN_CMD+XMODEM_LEN_SN+XMODEM_LEN_DAT+XMODEM_LEN_CRC)


#endif
