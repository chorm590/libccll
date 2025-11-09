#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "def.h"
#include "alloc.h"
#include "log.h"
#include "list.h"

TAG = "alloc";

static CRE_LIST_HEAD(l_objs);
typedef struct {
	char fun[24];
	char tag[24];
	int line;
	time_t tick;
	void *addr;
	size_t size; // On bytes
	LIST_HEAD list;
} Obj;
static const size_t obj_sz = sizeof(Obj);


Ret cl_alloc_init()
{
	// TODO
	return SUCC;
}

void * cl_malloc(const char *fun, const int line_no, const char *tag, int size_on_bytes)
{
	void *new_mem = malloc(size_on_bytes);
	if(new_mem == NULL)
	{
		CLOGE("malloc failed, err: %d", errno);
		return NULL;
	}

	Obj *obj = (Obj *) malloc(obj_sz);
	if(obj == NULL)
	{
		CLOGE("malloc obj failed, err: %d", errno);
		free(new_mem);
		return NULL;
	}
	sprintf(obj->fun, "%s", fun);
	sprintf(obj->tag, "%s", tag);
	obj->line = line_no;
	obj->tick = time(NULL);
	obj->addr = new_mem;
	obj->size = size_on_bytes;
	list_add(&obj->list, &l_objs);

	memset(new_mem, 0, size_on_bytes);

	return new_mem;
}

void cl_free(void *ptr)
{
	if(ptr == NULL) return;

	Obj *pos;
	list_for_each_entry(pos, &l_objs, list)
	{
		if(pos->addr == ptr)
		{
			free(ptr);
			list_del(&pos->list);
			free(pos);
			return;
		}
	}

	CLOGW("wild-ptr is freeing");
	free(ptr);
}

void cl_iter_objs()
{
#define PRT CLOGD
	PRT("Iterating the objs allocated:");
	PRT("  count: %d", list_size(&l_objs));
	Obj *pos;
	list_for_each_entry(pos, &l_objs, list)
	{
		PRT("  %ld [%s] %s+%d: addr: %p, size: %ld", pos->tick, pos->tag, pos->fun, pos->line, pos->addr, pos->size);
	}
	PRT("  ----");
#undef PRT
}

uint32_t cl_allocing_cnt()
{
	return list_size(&l_objs);
}
