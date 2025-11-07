#ifndef __CL_EVENT_H__
#define __CL_EVENT_H__

typedef struct CL_Event CL_Evt;
typedef void (*cl_evt_free)(void *data);
struct CL_Event {
	uint16_t no;
	void *data;
	cl_evt_free free_fun;
	LIST_HEAD list;
};

typedef Bool (*cl_evt_cb)(uint16_t evt_no, void *data);

Ret cl_evt_pub(uint16_t evt_no, void *data, cl_evt_free free_fun);
Ret cl_evt_sub(uint16_t evt_no, cl_evt_cb cb);
Ret cl_evt_unsub(uint16_t evt_no, cl_evt_cb cb);

#endif

