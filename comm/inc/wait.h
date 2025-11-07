#ifndef __CL_WAIT_H__
#define __CL_WAIT_H__

#include <time.h>
#include <sys/time.h>

#define SLEEP_CTL(seconds, milliseconds) \
	{ \
		nanosleep(&(struct timespec) { \
			.tv_sec = seconds, \
			.tv_nsec = milliseconds * 1000000 \
		}, NULL); \
	}

#define SLEEP_MS(milliseconds) \
	SLEEP_CTL(0, milliseconds)

#define SLEEP(seconds) \
	SLEEP_CTL(seconds, 0)

#endif
