#include <stdio.h>

#include "def.h"
#include "ccll.h"
#include "log.h"

Ret cl_init(print_fun pfun)
{
	if(pfun == NULL) pfun = printf;
	if(log_init() != SUCC)
	{
		pfun("init log failed");
		return FAIL;
	}

	return SUCC;
}

