#include <stdio.h>
#include <stdint.h>

#include "def.h"
#include "_log.h"
#include "log.h"
#include "convt.h"
#include "alloc.h"

TAG = TAG_PREFIX "convt";

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

Ret cl_bytes_to_readable_str(size_t bytes, char *readable_str)
{
	if(readable_str == NULL) return FAIL;

	const char* units[] = {"B", "KB", "MB", "GB"};
	int unit_index = 0;
	double size = bytes;

	while (size >= 1024 && unit_index < 3)
	{
		size /= 1024;
		unit_index++;
	}

	if (unit_index == 0)
		sprintf(readable_str, "%ld %s", bytes, units[unit_index]);
	else if (size < 10)
		sprintf(readable_str, "%.2f %s", size, units[unit_index]);
	else if (size < 100)
		sprintf(readable_str, "%.1f %s", size, units[unit_index]);
	else
		sprintf(readable_str, "%.0f %s", size, units[unit_index]);

	return SUCC;
}

