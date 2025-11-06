#ifndef __CL_WAIT_H__
#define __CL_WAIT_H__

#include <sys/time.h>

#define SLEEP(seconds, milliseconds) \
	{ \
		nanosleep(&(struct timespec) { \
			.tv_sec = seconds, \
			.tv_nsec = milliseconds * 1000000 \
		}, NULL); \
	}

#define SLEEP_MS(milliseconds) \
	SLEEP(0, milliseconds)

#define SLEEP_SEC(seconds) \
	SLEEP(seconds, 0)

#endif
