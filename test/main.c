#include <stdio.h>

#include "cl_def.h"
#include "cl_ccll.h"
#include "cl_log.h"

TAG = "test";

int main()
{
	if(cl_init(NULL) != SUCC)
	{
		printf("ccll init failed");
		return -1;
	}
	CLOGI("libccll init succ");

	cl_deinit();

	return 0;
}
