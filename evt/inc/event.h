#ifndef __CL_EVENT_H__
#define __CL_EVENT_H__

typedef void (*cl_evt_free)(void *data);
/*
 * @return  true -- evt intercepted, will not be publish to others from now.
 *         false -- keep spreading
 * */
typedef bool (*cl_evt_cb)(uint16_t evt_no, void *data);

/*
 * @data
 *    Must create a new heap memory.
 * @free_fun 
 *    if NULL, will free the 'data' with 'FREE' in 'cl_alloc.h'
 * */
Ret cl_evt_pub(uint16_t evt_no, void *data, cl_evt_free free_fun);
Ret cl_evt_sub(uint16_t evt_no, cl_evt_cb cb);
Ret cl_evt_unsub(uint16_t evt_no, cl_evt_cb cb);

#endif

