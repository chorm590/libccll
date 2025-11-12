#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "def.h"
#include "log.h"
#include "list.h"
#include "alloc.h"
#include "event.h"
#include "_event.h"
#include "wait.h"

TAG = "evt";

static sem_t l_sm_evt; // To indicate the event enqueue and dequeue.
static pthread_t l_thr_evt; // To manage the events.
static Bool l_run = false;
static pthread_mutex_t l_mtx_evt; // To synchronize the enqueue and dequeue of event.
static pthread_mutex_t l_mtx_sub;
static CRE_LIST_HEAD(l_li_evt); 
static CRE_LIST_HEAD(l_li_lsnrs);

static void * _evt_thread(void *data);

Ret cl_evt_init()
{
	TRACE();
	if(sem_init(&l_sm_evt, 0, 0))
	{
		CLOGE("init sm-evt failed, err: %d", errno);
		return FAIL;
	}

	if(pthread_create(&l_thr_evt, NULL, _evt_thread, NULL))
	{
		CLOGE("create thread for evt failed, err: %d", errno);
		sem_destroy(&l_sm_evt);
		return FAIL;
	}

	if(pthread_mutex_init(&l_mtx_evt, NULL))
	{
		CLOGE("init the mutex for event failed, err: %d", errno);
		SLEEP_MS(50); // to wait the sub-thread running
		cl_evt_deinit();
		return FAIL;
	}

	if(pthread_mutex_init(&l_mtx_sub, NULL))
	{
		CLOGE("init the mutex for sub failed, err: %d", errno);
		SLEEP_MS(50); // to wait the sub-thread running
		cl_evt_deinit();
		return FAIL;
	}

	return SUCC;
}

void cl_evt_deinit()
{
	TRACE();
	l_run = false;
	sem_post(&l_sm_evt);
	pthread_join(l_thr_evt, NULL);
	sem_destroy(&l_sm_evt);
	pthread_mutex_destroy(&l_mtx_evt);
	pthread_mutex_destroy(&l_mtx_sub);
}

Ret cl_evt_pub(uint16_t evt_no, void *data, cl_evt_free free_fun)
{
	TRACE();
	if(pthread_mutex_lock(&l_mtx_evt))
	{
		CLOGE("lock for pub-evt failed");
		return FAIL;
	}

	Ret ret = SUCC;
	CL_EVT *new_evt = MALLOC(sizeof(CL_EVT));
	if(new_evt == NULL)
	{
		CLOGE("malloc failed");
		ret = FAIL;
		goto UNLOCK7414;
	}
	new_evt->no = evt_no;
	new_evt->data = data;
	new_evt->free_fun = free_fun;
	list_add(&new_evt->list, &l_li_evt);

UNLOCK7414:
	if(pthread_mutex_unlock(&l_mtx_evt))
	{
		CLOGE("unlock for pub-evt failed");
		return FAIL;
	}

	if(ret == SUCC) sem_post(&l_sm_evt);

	return ret;
}

Ret cl_evt_sub(uint16_t evt_no, cl_evt_cb callback)
{
	TRACE();
	if(callback == NULL)
	{
		CLOGE("null 'callback'");
		return FAIL;
	}

	if(pthread_mutex_lock(&l_mtx_sub))
	{
		CLOGE("lock for sub-evt failed, err: %d", errno);
		return FAIL;
	}

	CL_EVT_LSNR *lsnr;
	list_for_each_entry(lsnr, &l_li_lsnrs, list)
	{
		if(lsnr->no == evt_no && lsnr->cb == callback)
		{
			CLOGW("Existing sub for evt-%d", evt_no);
			goto UNLOCK1146;
		}
	}

	// Create subscribe node
	CL_EVT_LSNR *new_lsnr = (CL_EVT_LSNR *) MALLOC(sizeof(CL_EVT_LSNR));
	if(new_lsnr == NULL)
	{
		CLOGE("malloc failed");
		goto UNLOCK1146;
	}
	new_lsnr->no = evt_no;
	new_lsnr->cb = callback;
	// no need to initilize the 'list'

	// Register the subscribe
	list_add(&new_lsnr->list, &l_li_lsnrs);
	CLOGD("new subscriber %d registered", new_lsnr->no);

UNLOCK1146:
	if(pthread_mutex_unlock(&l_mtx_sub))
	{
		CLOGE("unlock for sub-evt failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}

Ret cl_evt_unsub(uint16_t evt_no, cl_evt_cb callback)
{
	TRACE();
	if(callback == NULL)
	{
		CLOGE("null 'callback'");
		return FAIL;
	}

	if(pthread_mutex_lock(&l_mtx_sub))
	{
		CLOGE("lock for unsub-evt failed, err: %d", errno);
		return FAIL;
	}

	CL_EVT_LSNR *lsnr = NULL;
	Bool found = false;
	list_for_each_entry(lsnr, &l_li_lsnrs, list)
	{
		if(lsnr->no == evt_no && lsnr->cb == callback)
		{
			found = true;
			break;
		}
	}

	if(found)
	{
		list_del(&lsnr->list);
		FREE(lsnr);
		CLOGD("subscribe %d removed", evt_no);
	}

	if(pthread_mutex_unlock(&l_mtx_sub))
	{
		CLOGE("unlock for unsub-evt failed, err: %d", errno);
		return FAIL;
	}

	return found ? SUCC : FAIL;
}

static void * _evt_thread(void *data)
{
	TRACE();
	l_run = true;
	while(l_run)
	{
		// 1. wait the signal
		sem_wait(&l_sm_evt);
		CLOGD("new evt");

		if(pthread_mutex_lock(&l_mtx_evt))
		{
			CLOGE("lock the evt-thrd failed, err: %d", errno);
			break;
		}

		// 2. popup an event
		CL_EVT *evt = NULL;
		if(l_li_evt.next != &l_li_evt)
		{
			evt = container_of(l_li_evt.next, CL_EVT, list);
			list_del(&evt->list);
		}

		if(pthread_mutex_unlock(&l_mtx_evt))
		{
			CLOGE("unlock the evt-thrd failed, err: %d", errno);
			break;
		}

		// 3. notify the listeners
		if(evt)
		{
			CL_EVT_LSNR *lsnr;
			list_for_each_entry(lsnr, &l_li_lsnrs, list)
			{
				if(lsnr->no == evt->no && lsnr->cb(evt->no, evt->data))
				{
					CLOGI("evt %d was consumed", lsnr->no);
					break;
				}
			}
		}

		// 4. release the event
		if(evt)
		{
			if(evt->free_fun) evt->free_fun(evt->data);
			else if(evt->data) FREE(evt->data);
			FREE(evt);
		}
	}
	CLOGE("Fatal err: out of the evt-thrd");

	return NULL;
}
