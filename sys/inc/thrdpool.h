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

typedef void (*work_fun)(void *args);

/*
 * Post a worker fun, the 'fun' will be execute in a random sub-thread in pool
 * refering by 'id'. It will auto balance the execution on each pool-thread.
 * 
 * Don't waste too much time at 'fun'.
 *
 * */
Ret cl_trpo_post(int id, work_fun fun, void *args);

#endif
