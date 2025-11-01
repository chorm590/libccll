#include <stdlib.h>
#include <errno.h>

#include "alloc.h"
#include "log.h"

TAG = "alloc";

void * cl_malloc(const char *fun, const int line_no, const char *tag, int size_on_bytes)
{
	void * new_mem = malloc(size_on_bytes);
	if(new_mem == NULL)
	{
		CLOGE("malloc failed, err: %d", errno);
		return NULL;
	}

	return new_mem;
}

void cl_free(void *ptr)
{
	if(ptr)
	{
		free(ptr);
	}
}
