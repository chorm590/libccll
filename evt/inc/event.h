#ifndef __CL_EVENT_H__
#define __CL_EVENT_H__

typedef Bool (*cl_evt_cb)();
typedef void (*cl_evt_free)();

Ret cl_evt_pub();
Ret cl_evt_sub();
Ret cl_evt_unsub();

#endif

