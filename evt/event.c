#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "def.h"
#include "log.h"
#include "event.h"
#include "_event.h"
#include "wait.h"

TAG = "evt";

static sem_t l_sm_evt; // To indicate the event enqueue and dequeue.
static pthread_t l_thr_evt; // To manage the events.
static Bool l_run = false;
static pthread_mutex_t l_mtx_evt; // To synchronize the enqueue and dequeue of event.
static CRE_LIST_HEAD(l_li_evt); 
static CRE_LIST_HEAD(l_li_lsnrs);

Ret cl_evt_init()
{
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

	return SUCC;
}

void cl_evt_deinit()
{
	TRACE();
	l_run = false;
	sem_post(&l_sm_evt);
	pthread_join(&l_thr_evt);
	sem_destroy(&l_sm_evt);
}

static void * _evt_thread(void *data)
{
	l_run = true;
	while(l_run)
	{
		// 1. wait the signal
		sem_wait(&l_sm_evt);
		if(pthread_mutex_lock(&l_mtx_evt))
		{
			CLOGE("lock the evt-thrd failed, err: %d", errno);
			break;
		}

		// 2. popup an event
		CL_Evt *evt = NULL;
		if(l_li_evt.next != &l_li_evt)
		{
			evt = container_of(l_li_evt.next, CL_Evt, list);
		}

		if(pthread_mutex_unlock(&l_mtx_evt))
		{
			CLOGE("unlock the evt-thrd failed, err: %d", errno);
			break;
		}

		// 3. notify the listeners
		if(evt)
		{
			CL_evt_lsnrs *lsnr;
			list_for_each_entry(lsnr, l_li_lsnrs, list)
			{
				if(lsnr->cb(evt->no, evt->data))
				{
					CLOGI("evt %d was consumed");
					break;
				}
			}
		}

		// 4. release the event
		if(evt)
		{
			if(evt->free_fun) evt->free_fun(evt);
		}
	}
	return NULL;
}
