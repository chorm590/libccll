#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "def.h"
#include "_def.h"
#include "log_type.h"
#include "_log.h"
#include "log.h"

TAG = "io";

Ret cl_mkdir_p(const char *path, const mode_t mode)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);

	if(tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	for(p = tmp + 1; *p; p++)
	{
		if(*p == '/')
		{
			*p = '\0';
			if(mkdir(tmp, 0755) != 0)
				if(errno != EEXIST)
				{
					CLOGE("cre dir failed, err: %d", errno);
					return FAIL;
				}
			*p = '/';
		}
	}

	if(mkdir(tmp, mode) != 0)
		if (errno != EEXIST)
		{
			CLOGE("cre dir failed, err: %d", errno);
			return FAIL;
		}

	return SUCC;
}

