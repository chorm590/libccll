#include <stdio.h>
#include <stdint.h>

#include "_def.h"
#include "def.h"
#include "ccll.h"
#include "_log.h"
#include "log.h"
#include "_alloc.h"
#include "list.h"
#include "event.h"
#include "_event.h"
#include "_timer.h"
#include "timer.h"

TAG = "main";

int s_init = false;
print_fun s_prtfun;

#define INIT(what) \
	if(cl_##what##_init() != SUCC) \
	{ \
		fprintf(stderr, "init " #what " failed\n"); \
		return FAIL; \
	}

#define DEINIT(what) \
	cl_##what##_deinit();


Ret cl_init(print_fun pfun)
{
	if(s_init) return SUCC;
	s_prtfun = NULL;
	if(pfun == NULL)
	{
		pfun = cl_log_get_def_prtfun();
		printf("using def-print-fun\n");
	}

	INIT(log);
	INIT(alloc);
	INIT(evt);
	INIT(timer);

	s_prtfun = pfun;
	s_init = true;
	cl_log(DEBUG, cltag, "libccll initialized");
	return SUCC;
}

void cl_deinit()
{
	s_init = false;
	s_prtfun = NULL;
	DEINIT(log);
	// TODO
	printf("libccll de-initialized\n");
}

