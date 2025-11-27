#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "def.h"
#include "alloc.h"
#include "log_type.h"
#include "_log.h"
#include "log.h"
#include "list.h"

TAG = TAG_PREFIX "alloc";

static CRE_LIST_HEAD(g_li_objs);
static pthread_mutex_t g_mtx_objs = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
	char fun[24];
	char tag[24];
	int line;
	time_t tick;
	void *addr;
	size_t size; // On bytes
	CLIST list;
} Obj;
static const size_t obj_sz = sizeof(Obj);


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
	CL_LOCK(&g_mtx_objs);
	list_add(&obj->list, &g_li_objs);
	CL_UNLOCK(&g_mtx_objs);

	memset(new_mem, 0, size_on_bytes);

	return new_mem;
}

void cl_free(void *ptr)
{
	if(ptr == NULL) return;

	Obj *pos;
	CL_LOCK(&g_mtx_objs);
	list_for_each2(pos, &g_li_objs, list)
	{
		if(pos->addr == ptr)
		{
			free(ptr);
			list_del(&pos->list);
			free(pos);
			CL_UNLOCK(&g_mtx_objs);
			return;
		}
	}
	CL_UNLOCK(&g_mtx_objs);

	CLOGW("wild-ptr is freeing");
	free(ptr);
}

void cl_iter_objs()
{
#define PRT CLOGD
	PRT("Iterating the objs allocated:");
	CL_LOCK(&g_mtx_objs);
	PRT("  count: %d", list_size(&g_li_objs));
	Obj *pos;
	list_for_each2(pos, &g_li_objs, list)
	{
		PRT("  %ld [%s] %s+%d: addr: %p, size: %ld", pos->tick, pos->tag, pos->fun, pos->line, pos->addr, pos->size);
	}
	PRT("  ----");
	CL_UNLOCK(&g_mtx_objs);
#undef PRT
}

uint32_t cl_allocing_cnt()
{
	CL_LOCK(&g_mtx_objs);
	const size_t cnt = list_size(&g_li_objs);
	CL_UNLOCK(&g_mtx_objs);

	return cnt;
}

size_t cl_allocing_bytes()
{
	CL_LOCK(&g_mtx_objs);
	size_t bytes = 0;
	Obj *obj;
	list_for_each2(obj, &g_li_objs, list)
	{
		bytes += obj->size;
	}
	CL_UNLOCK(&g_mtx_objs);

	return bytes;
}

void cl_alloc_deinit()
{
	pthread_mutex_destroy(&g_mtx_objs);
}

