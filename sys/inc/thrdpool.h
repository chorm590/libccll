#ifndef __CL_THRDPOOL_H__
#define __CL_THRDPOOL_H__

/*
 * Create a new thread-pool
 *
 * @param name [in]
 * 		  Should less than 16 bytes
 *
 * @return On success, will returns the pool id which is equals or greater than zero.
 * 		   On error, -1 will be returns.
 * */
int cl_trpo_create(int amount, const char *name);

/*
 * @param id [in]
 * 		  The return num in 'cl_trpo_create'
 * */
void cl_trpo_destroy(int id);

typedef struct {
	int thrd_id;
	void *args; // from 'cl_trpo_post' param 3
} CLTrPoArg;

/*
 * @param args
 * 		  No need to free the 'args', but you may free the 'args->args',
 * 		  that's your data you posted.
 * */
typedef void (*work_fun)(CLTrPoArg *args);

/*
 * Post a worker fun, the 'fun' will be execute in a random sub-thread in pool
 * refering by 'id'. It will auto balance the execution on each pool-thread.
 * 
 * Don't waste too much time at 'fun'.
 *
 * */
Ret cl_trpo_post(int id, work_fun fun, void *args);

#endif
