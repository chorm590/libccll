#include <stdio.h>

#include "_def.h"
#include "def.h"
#include "ccll.h"
#include "_log.h"
#include "log.h"

TAG = "main";

int s_init = false;
print_fun s_prtfun;

#define INIT(what) \
	if(what##_init() != SUCC) \
	{ \
		fprintf(stderr, "init " #what " failed\n"); \
		return FAIL; \
	}


Ret cl_init(print_fun pfun)
{
	if(s_init) return SUCC;
	s_prtfun = NULL;
	if(pfun == NULL)
	{
		pfun = log_get_def_prtfun();
		printf("using def-print-fun\n");
	}

	INIT(log);
	INIT(alloc);

	s_prtfun = pfun;
	s_init = true;
	cl_log(DEBUG, cltag, "libccll initialized");
	return SUCC;
}

void cl_deinit()
{
	s_init = false;
	s_prtfun = NULL;
	printf("libccll de-initialized\n");
}

