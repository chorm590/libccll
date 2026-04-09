#ifndef __CL_EVENT_H__
#define __CL_EVENT_H__

typedef void (*cl_evt_free)(void *data);
static inline void cl_evt_fake_free_fun(void *data) {
	// Do nothing
}
/*
 * @return  true -- evt intercepted, will not be publish to others from now.
 *         false -- keep spreading
 * */
typedef bool (*cl_evt_cb)(uint16_t evt_no, void *data);

Ret cl_evt_pub(uint16_t evt_no, void *data, cl_evt_free free_fun);
Ret cl_evt_sub(uint16_t evt_no, cl_evt_cb cb);
Ret cl_evt_unsub(uint16_t evt_no, cl_evt_cb cb);

#endif

