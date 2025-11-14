#include <string.h>

#include "def.h"
#include "log.h"
#include "txt.h"

TAG = "txt";


int cl_txt_pos_line(char *dst, const size_t max, char *src)
{
	TRACE();
	if(src == NULL || dst == NULL || max < 1) return -1;

	int off = 0;
	char *_src = src;
	while(*_src != 0)
	{
		if(*_src == '\n')
		{
			strncpy(dst, src, off);
			*(dst + off) = 0;
			if(off > 0 && *(dst + off - 1) == '\r')
			{
				*(dst + off - 1) = 0;
			}
			return off + 1;
		}

		_src++;
		off++;
	}

	strncpy(dst, src, off);
	*(dst + off) = 0;

	return 0;
}

