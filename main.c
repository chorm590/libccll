#include <stdio.h>
#include <stdint.h>

#include "_def.h"
#include "def.h"
#include "ccll.h"
#include "_log.h"
#include "log.h"
#include "list.h"
#include "event.h"
#include "_event.h"
#include "_timer.h"
#include "timer.h"
#include "_rsa.h"
#include "_klciph.h"

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
	INIT(evt);
	INIT(timer);
	INIT(rsa);
	INIT(klciph); // Optional fun

	s_prtfun = pfun;
	s_init = true;
	cl_log(DEBUG, cltag, "libccll initialized");

	return SUCC;
}

void cl_deinit()
{
	TRACE();
	s_init = false;
	s_prtfun = NULL;
	DEINIT(klciph);
	DEINIT(rsa);
	DEINIT(timer);
	DEINIT(evt);
	DEINIT(log);
	printf("libccll de-initialized\n");
}

