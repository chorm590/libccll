#include <stdio.h>
#include <stdint.h>

#include "def.h"
#include "log.h"
#include "bytes.h"
#include "alloc.h"

TAG = "bytes";

void cl_print_bytes(uint8_t *bytes, int size)
{
	if(bytes == NULL || size > 4096) return;

	char *buf = (char *) MALLOC((size << 1) + 256/*16 '\n'*/);
	int i, j, k;
	for(i = 0, j = 0, k = 0; i < size; i++, k++)
	{
		if(k == 15)
		{
			k = -1;
			sprintf(buf + j, "%02x\n", *(bytes + i));
			j += 3;
		}
		else
		{
			sprintf(buf + j, "%02x", *(bytes + i));
			j += 2;
		}
	}
	CLOGI("\n%s\n", buf);
	FREE(buf);
}

