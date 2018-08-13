
#if 1
#include "device.h"
#include "uart.h"
#include "bsp.h"
void Device_Init(void)
{
	return;
}

/*
** client: type=1, servie: type=0
*/
INT32 Device_Open(UINT8 type, UINT8 *ip, UINT16 port)
{
	return 1;
}

INT32 Device_Send(INT32 d, UINT8 *src, INT32 len, INT32 timeout)
{
	//return USBD_Write(src, len);  todo
    return UART_appWrite(src, len);
}

INT32 Device_Recv(INT32 d, UINT8 *dst, INT32 len, INT32 timeout)
{
	//return USBD_Read(dst, len); todo
    return UART_appRead(dst, len);
}

INT32 Device_Select(INT32 d, INT32 timeout)
{
	return 1;
}

void Device_Close(INT32 d)
{
	return;
}

#else

#include "device.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_MAX_DEV		4

typedef struct{
	INT32 sn;
	INT32 sk;
	INT32 inuse;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
}device_t;

device_t device[NUM_MAX_DEV];

void Device_Init(void)
{
	INT32 i;
	
	for(i = 0 ; i < NUM_MAX_DEV; i++)
	{
		device[i].sn = i+1;
		device[i].sk = -1;
		device[i].inuse = 0;
		memset((void *)&device[i].local_addr, 0, sizeof(struct sockaddr_in));
		memset((void *)&device[i].remote_addr, 0, sizeof(struct sockaddr_in));
	}
}

/*
** client: type=1, servie: type=0
*/
INT32 Device_Open(UINT8 type, UINT8 *ip, UINT16 port)
{
	INT32 i;
	INT32 dd = 0;
	INT32 sock = 0;
	
	for(i = 0; i < NUM_MAX_DEV; i++)
	{
		if(device[i].inuse == 0)
		{
			break;
		}
	}
	
	if(i == NUM_MAX_DEV)
	{
		goto done;
	}
	
	if((sock=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("socket error.\n");
		goto done;
	}	

	if(type == 0) //server
	{
		device[i].local_addr.sin_family = AF_INET;
		device[i].local_addr.sin_port = htons(port);
		device[i].local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(bind(sock, (struct sockaddr *)&device[i].local_addr, sizeof(device[i].local_addr)) < 0)
		{
			close(sock);
			goto done;
		}
		printf("server port = 0x%08X.\r\n", device[i].local_addr.sin_port);
	}
	else //client
	{
		device[i].remote_addr.sin_family = AF_INET;
		device[i].remote_addr.sin_port = htons(port);
		device[i].remote_addr.sin_addr.s_addr = inet_addr(ip);
		printf("server portttt = 0x%08X.\r\n", device[i].remote_addr.sin_port);
	}
	
	device[i].sk = sock;
	device[i].inuse = 1;
	dd = device[i].sn;
	
done:
	return dd;
}

INT32 Device_Send(INT32 d, UINT8 *src, INT32 len, INT32 timeout)
{
	char *addr;
	addr = inet_ntoa(device[d-1].remote_addr.sin_addr);
	printf("sendto %s(0x%04X) %d byte(s).\n", addr, device[d-1].remote_addr.sin_port, len);
	return sendto(device[d-1].sk, src, len , 0, \
			(struct sockaddr *)&device[d-1].remote_addr, \
			sizeof(struct sockaddr_in));
}

INT32 Device_Recv(INT32 d, UINT8 *dst, INT32 len, INT32 timeout)
{
	INT32 ret = 0;
	INT32 addr_len = sizeof(struct sockaddr_in);
	char *addr;

	printf("recv\n");
	ret = recvfrom(device[d-1].sk, dst, len, 0, \
					(struct sockaddr *)&device[d-1].remote_addr, &addr_len);
	addr = inet_ntoa(device[d-1].remote_addr.sin_addr);
	printf("from %s(0x%04X) %d byte(s).\n", addr, device[d-1].remote_addr.sin_port, ret);

	return ret;
}

INT32 Device_Select(INT32 d, INT32 timeout)
{
#if 0
	fd_set rfd;
	struct timeval timeout;

	timeout.tv_sec = ns;
	timeout.tv_usec = 0;

	FD_ZERO(&rfd);
	FD_SET(device[d-1].sk, &rfd);

	return select(1, &rfd, NULL, NULL, &timeout);
#else
	return 1;
#endif
}

void Device_Close(INT32 d)
{
	close(device[d-1].sk);
	device[d-1].inuse = 0;
	device[d-1].sk = -1;
	
	memset(&device[d-1].local_addr, 0, sizeof(struct sockaddr_in));
	memset(&device[d-1].remote_addr, 0, sizeof(struct sockaddr_in));
}
#endif
