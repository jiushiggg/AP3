
#include "coremem.h"
#include "datatype.h"
#include "updata.h"

//#define COREMEM_SIZE	14000//(1024*14)

static UINT8 coremem_inuse = 0;

//#pragma location = (0x200001800);
//static UINT8 coremem[COREMEM_SIZE] = {0};
static UINT8 coremem[COREMEM_SIZE] = {0};

void *Core_Malloc(UINT32 size)
{
	if(coremem_inuse == 1)
	{
		return NULL;
	}
	else if(size > COREMEM_SIZE)
	{
		return NULL;
	}
	else
	{
//		coremem_inuse = 1;
		return coremem;
	}
}

void Core_Free(void *ptr)
{
	coremem_inuse = 0;
}




