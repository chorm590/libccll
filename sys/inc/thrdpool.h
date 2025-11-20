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

#endif
