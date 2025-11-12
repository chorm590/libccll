#ifndef __CL_DEF_H__
#define __CL_DEF_H__

#include <stddef.h>

typedef enum {
	FALSE = (1 == 0),
	TRUE = (1 == 1)
} Bool;
#define true TRUE
#define false FALSE

typedef enum {
	FAIL = -1,
	SUCC
} Ret;

#define LOG_INIT_FAIL(module) \
	CLOGE("init the " #module " failed, err: %d", errno)

#define LOG_LOCK_FAIL(module) \
	CLOGE("lock for the " #module " failed, err: %d", errno)

#define LOG_UNLOCK_FAIL(module) \
	CLOGE("unlock for the " #module " failed, err: %d", errno)

#define container_of(ptr, type, member) ({ \
		const typeof(((type *) 0)->member) *__mptr = (ptr); \
		(type *) ((char *) __mptr - offsetof(type, member)); \
		})

#endif

