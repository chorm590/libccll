#ifndef __CL__EVENT_H__
#define __CL__EVENT_H__

typedef struct {
	uint16_t no;
	cl_evt_cb cb;
	CLIST list;
} CL_EVT_LSNR;

Ret cl_evt_init();
void cl_evt_deinit();

#endif
