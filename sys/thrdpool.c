#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "def.h"
#include "log_type.h"
#include "_log.h"
#include "log.h"
#include "list.h"
#include "thrdpool.h"
#include "alloc.h"

TAG = TAG_PREFIX "thrdpool";

typedef (*work_fun)(void *args);
typedef struct {
	work_fun wkfn;
	CLIST list;
} WorkFun;

typedef struct {
	pthread_t ptrdt;
	sem_t smp;
	CLIST wkfn_list; // list head of WorkFun
} PolThrd;

typedef struct {
	int id;
	char name[17];
	int amt;
	PolThrd *pts;
	CLIST list;
} Pool;
static CRE_LIST_HEAD(g_li_pols);
static pthread_mutex_t g_mtx_pols = PTHREAD_MUTEX_INITIALIZER;

#define MAX_THRD_CNT_SHIFT_BIT 2 // max = nprocs << SHIFT_BIT
#define MAX_THRD_CNT_CRE 256 // maximum thread request at a time
#define DEF_TRPO_NAME "cltrpol"

static int _get_cur_thrd_cnt()
{
	int cnt = 0;
	Pool *pool;
	list_for_each_entry(pool, &g_li_pols, list)
	{
		cnt += pool->amt;
	}

	return cnt;
}

static int _cre_pol_id()
{
	int i = 0, id = -1;
	Bool using;
	Pool *pool;
	for(; i < 10000; i++)
	{
		using = false;
		list_for_each_entry(pool, &g_li_pols, list)
		{
			if(pool->id == i)
			{
				using = true;
				break;
			}
		}

		if(!using)
		{
			id = i;
			break;
		}
	}

	return id;
}

static void * _worker_thread(void *ptr)
{
	PolThrd *potr = (PolThrd *) ptr;

	while(true)
	{
		sem_wait(&potr->smp);
	}

	return NULL;
}

static Ret _cre_thrds(PolThrd *ptr, int amt)
{
	int i = 0;
	// Init the mutex and sem_t
	for(; i < amt; i++)
	{
		if(pthread_mutex_init(&(ptr + i)->ptrdt, NULL))
		{
			CLOGE("[%d] init mutex failed, err: %d", i, errno);
			for(i--; i > -1; i--)
				if(pthread_mutex_destroy(&(ptr + i)->ptrdt))
					CLOGE("[%d] deinit mutex failed, err: %d", i, errno);
			return FAIL;
		}
		init_list_node(&(ptr + i)->wkfn_list);
		if(sem_init(&(ptr + i)->smp, 0, 0))
		{
			CLOGE("[%d] init sem_t failed, err: %d", i, errno);
			for(i--; i > -1; i--)
			{
				if(pthread_mutex_destroy(&(ptr + i)->ptrdt))
					CLOGE("[%d] deinit mutex failed, err: %d", i, errno);
				if(sem_destroy(&(ptr + i)->smp))
					CLOGE("[%d] destroy sem_t failed, err: %d", i, errno);
			}
			return FAIL;
		}
	}

	// Create the sub-threads
	for(i = 0; i < amt; i++)
	{
		if(pthread_create(&(ptr + i)->ptrdt, NULL, _worker_thread, ptr + i))
		{
			CLOGE("[%d] cre thread failed, err: %d", i, errno);
			return FAIL;
		}
	}
	

	return SUCC;
}

int cl_trpo_create(int amount, const char *name)
{
	if(amount > MAX_THRD_CNT_CRE)
	{
		CLOGE("amount limit exceed, max: %d", MAX_THRD_CNT_CRE);
		return -1;
	}

	if(pthread_mutex_lock(&g_mtx_pols))
	{
		CLOGE("lock failed");
		return -1;
	}

	const int cur_nrs = _get_cur_thrd_cnt();
	const int max_thrd_lmt = get_nprocs_conf() << MAX_THRD_CNT_SHIFT_BIT;
	if(cur_nrs >= max_thrd_lmt)
	{
		CLOGW("thrd limit exceed, %d/%d", cur_nrs, max_thrd_lmt);
		return -1;
	}

	const int id = _cre_pol_id();
	if(id == -1)
	{
		CLOGE("req id failed");
		return -1;
	}
	Pool *pool = (Pool *) MALLOC(sizeof(Pool));
	pool->id = id;
	if(name && *name != 0) sprintf(pool->name, "%s", name);
	else sprintf(pool->name, DEF_TRPO_NAME);
	pool->amt = amount;
	pool->pts = (PolThrd *) MALLOC(sizeof(PolThrd) * amount);

	if(_cre_thrds(pool->pts, pool->amt) != SUCC)
	{
		CLOGE("cre thread failed");
		FREE(pool->pts); // don't worry about the wkfn_list.
						 // in cre-fun, that list must be empty!
		FREE(pool);
		pthread_mutex_unlock(&g_mtx_pols);
		return -1;
	}

	list_add(&pool->list, &g_li_pols);

	if(pthread_mutex_unlock(&g_mtx_pols))
	{
		CLOGE("unlock failed, err: %s", strerror(errno));
		FREE(pool->pts);
		FREE(pool);
		return -1;
	}

	return id;
}

void cl_trpo_destroy(int id)
{
	if(pthread_mutex_lock(&g_mtx_pols))
	{
		CLOGE("lock failed");
		return;
	}

	Bool found = false;
	Pool *pool;
	list_for_each_entry(pool, &g_li_pols, list)
	{
		if(pool->id == id)
		{
			list_del(&pool->list);
			found = true;
		}
	}

	if(pthread_mutex_unlock(&g_mtx_pols))
	{
		CLOGE("unlock failed");
	}

	if(found)
	{
		// 1. join all the threads
		int i = 0;
		for(; i < pool->amt; i++)
		{
			// TODO
			pthread_join((pool->pts + i)->ptrdt, NULL);
		}

		// 2. free pthread_t
		FREE(pool->pts);

		// 3. free Pool
		FREE(pool);
	}
}

Ret cl_trpo_init()
{
	return SUCC;
}

void cl_trpo_deinit()
{
	
	pthread_mutex_destroy(&g_mtx_pols);
}

