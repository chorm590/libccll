#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
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
#include "wait.h"

TAG = TAG_PREFIX "thrdpool";

typedef struct {
	work_fun wkfn;
	void *args;
	CLIST list;
} WorkFun;

typedef struct {
	pthread_t ptrdt;
	pthread_mutex_t mtx;
	sem_t smp;
	Bool exit;
	int *run_pt; // refer to Pool.run_pt
	CLIST wkfn_list; // list head of WorkFun
} PolThrd;

typedef struct {
	int id;
	char name[17];
	int amt;
	PolThrd *pts;
	int run_pt; // The cnt of running pol-thrd
	CLIST list;
} Pool;
static CRE_LIST_HEAD(g_li_pols);
static pthread_mutex_t g_mtx_pols = PTHREAD_MUTEX_INITIALIZER;
static Bool g_init = false;

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
	PolThrd *cur = (PolThrd *) ptr;
	__atomic_fetch_add(cur->run_pt, 1, __ATOMIC_RELAXED);

	while(true)
	{
		sem_wait(&cur->smp);
		// 1. Check wheter requsting exit
		if(cur->exit)
		{
			CLOGW("pol-thrd exiting");
			CL_LOCK(&cur->mtx);
			const size_t cnt = list_size(&cur->wkfn_list);
			int i = 0;
			for(; i < cnt; i++)
			{
				WorkFun *wf;
				list_for_each_entry(wf, &cur->wkfn_list, list)
				{
					FREE(wf);
					// Bug: wr->args cannot be free
					break;
				}
			}
			CL_UNLOCK(&cur->mtx);
			break;
		}

		// 2. Get the first enqueue element
		CL_LOCK(&cur->mtx);
		WorkFun *wf = NULL;
		list_for_each_entry(wf, &cur->wkfn_list, list)
		{
			break;
		}
		CL_UNLOCK(&cur->mtx);

		// 3. Execute it and then remove it
		if(wf)
		{
			if(wf->wkfn) wf->wkfn(wf->args);
			CL_LOCK(&cur->mtx);
			list_del(&wf->list);
			CL_UNLOCK(&cur->mtx);
			FREE(wf);
		}
	}

	CLOGW("pol-thrd exited");

	return NULL;
}

static Ret _cre_thrds(PolThrd *ptr, int amt, int *run_pt)
{
	int i = 0;
	// Init the mutex and sem_t
	for(; i < amt; i++)
	{
		PolThrd *cur = ptr + i;
		if(pthread_mutex_init(&cur->mtx, NULL))
		{
			CLOGE("[%d] init mutex failed, err: %d", i, errno);
			exit(1);
		}
		init_list_node(&cur->wkfn_list);
		if(sem_init(&cur->smp, 0, 0))
		{
			CLOGE("[%d] init sem_t failed, err: %d", i, errno);
			exit(1);
		}
		cur->exit = false;
		cur->run_pt = run_pt;
	}

	// Create the sub-threads
	for(i = 0; i < amt; i++)
	{
		if(pthread_create(&(ptr + i)->ptrdt, NULL, _worker_thread, ptr + i))
		{
			CLOGE("[%d] cre thread failed, err: %d", i, errno);
			exit(1);
		}
	}

	return SUCC;
}

int cl_trpo_create(int amount, const char *name)
{
	TRACE();
	if(!g_init) return -1;
	if(amount < 1 || amount > MAX_THRD_CNT_CRE)
	{
		CLOGE("amount limit exceed: [1, %d]", MAX_THRD_CNT_CRE);
		return -1;
	}

	CL_LOCK(&g_mtx_pols);

	const int cur_nrs = _get_cur_thrd_cnt();
	const int max_thrd_lmt = get_nprocs_conf() << MAX_THRD_CNT_SHIFT_BIT;
	if(cur_nrs >= max_thrd_lmt)
	{
		CLOGW("thrd limit exceed, %d/%d", cur_nrs, max_thrd_lmt);
		CL_UNLOCK(&g_mtx_pols);
		return -1;
	}
	else if((cur_nrs + amount) > max_thrd_lmt)
	{
		CLOGW("res not enough, cur: %d/%d, req: %d", cur_nrs, max_thrd_lmt, amount);
		CL_UNLOCK(&g_mtx_pols);
		return -1;
	}

	const int id = _cre_pol_id();
	if(id == -1)
	{
		CLOGE("req id failed");
		CL_UNLOCK(&g_mtx_pols);
		return -1;
	}
	Pool *pool = (Pool *) MALLOC(sizeof(Pool));
	pool->id = id;
	if(name && *name != 0) sprintf(pool->name, "%s", name);
	else sprintf(pool->name, DEF_TRPO_NAME);
	pool->amt = amount;
	pool->pts = (PolThrd *) MALLOC(sizeof(PolThrd) * amount);
	pool->run_pt = 0;

	if(_cre_thrds(pool->pts, pool->amt, &pool->run_pt) != SUCC)
	{
		CLOGE("cre thread failed");
		FREE(pool->pts); // don't worry about the wkfn_list.
						 // in cre-fun, that list must be empty!
		FREE(pool);
		CL_UNLOCK(&g_mtx_pols);
		return -1;
	}

	list_add(&pool->list, &g_li_pols);

	while(true)
	{
		if(__atomic_load_n(&pool->run_pt, __ATOMIC_RELAXED) == amount) break;
		SLEEP_MS(10);
	}

	CL_UNLOCK(&g_mtx_pols);

	return id;
}

static Pool * _pop_a_pool(int id)
{
	Bool found = false;
	Pool *pool;
	list_for_each_entry(pool, &g_li_pols, list)
	{
		if(pool->id == id)
		{
			list_del(&pool->list);
			found = true;
			break;
		}
	}

	return found ? pool : NULL;
}

static void _destroy_pol_thrd(Pool *pool)
{
	int i = 0;
	for(; i < pool->amt; i++)
	{
		PolThrd *pt = pool->pts + i;
		CL_LOCK(&pt->mtx);
		pt->exit = true;
		sem_post(&pt->smp);
		CL_UNLOCK(&pt->mtx);
		SLEEP_MS(10);
		pthread_join(pt->ptrdt, NULL);
	}
}

void cl_trpo_destroy(int id)
{
	CL_LOCK(&g_mtx_pols);
	Pool *pool = _pop_a_pool(id);
	CL_UNLOCK(&g_mtx_pols);
	if(pool == NULL) return;

	_destroy_pol_thrd(pool);
	FREE(pool->pts); // The wk_fn will be release in _worker_thread
	FREE(pool);
}

Ret cl_trpo_init()
{
	g_init = true;
	return SUCC;
}

void cl_trpo_deinit()
{
	TRACE();
	g_init = false;

	CL_LOCK(&g_mtx_pols);
	const size_t sz = list_size(&g_li_pols);
	int i = 0;
	for(; i < sz; i++)
	{
		Pool *pool;
		list_for_each_entry(pool, &g_li_pols, list)
		{
			list_del(&pool->list);
			_destroy_pol_thrd(pool);
			FREE(pool->pts);
			FREE(pool);
			break;
		}
	}
	CL_UNLOCK(&g_mtx_pols);

	pthread_mutex_destroy(&g_mtx_pols);
}

Ret cl_trpo_post(int id, work_fun fun, void *args)
{
	TRACE();
	if(!g_init) return FAIL;
	CL_LOCK(&g_mtx_pols);

	Pool *po;
	Bool none = true;
	// Find the Pool
	list_for_each_entry(po, &g_li_pols, list)
	{
		if(po->id != id) continue;

		none = false;
		break;
	}

	if(none)
	{
		CLOGW("Invalid id");
		CL_UNLOCK(&g_mtx_pols);
		return FAIL;
	}
	// Select the pol-thrd
	PolThrd *pt = NULL;
	int i = 0, min = 10000;
	for(; i < po->amt; i++)
	{
		CL_LOCK(&(po->pts + i)->mtx);
		const size_t sz = list_size(&(po->pts + i)->wkfn_list);
		if(sz == 0)
		{
			pt = po->pts + i;
			CL_UNLOCK(&(po->pts + i)->mtx);
			break;
		}

		if(min > sz)
		{
			pt = po->pts + i;
			min = sz;
		}
		CL_UNLOCK(&(po->pts + i)->mtx);
	}

	// Post the worker fun
	WorkFun *wf = (WorkFun *) MALLOC(sizeof(WorkFun));
	wf->wkfn = fun;
	wf->args = args;
	CL_LOCK(&pt->mtx);
	list_add(&wf->list, &pt->wkfn_list);
	sem_post(&pt->smp);
	CL_UNLOCK(&pt->mtx);

	CL_UNLOCK(&g_mtx_pols);

	return SUCC;
}

