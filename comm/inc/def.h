#ifndef __CL_DEF_H__
#define __CL_DEF_H__

#include <stddef.h>
#include <stdint.h>

typedef char bool;
#define true 1
#define false 0

typedef enum {
	FAIL = -1,
	SUCC
} Ret;

#define container_of(ptr, type, member) ({ \
		const typeof(((type *) 0)->member) *__mptr = (ptr); \
		(type *) ((char *) __mptr - offsetof(type, member)); \
		})
#define CL_LOCK(mtx_pthrd) \
	if(pthread_mutex_lock((mtx_pthrd))) \
	{ \
		CLOGE("lock with mtx failed, err: %d", errno); \
		exit(124); \
	}
#define CL_UNLOCK(mtx_pthrd) \
	if(pthread_mutex_unlock((mtx_pthrd))) \
	{ \
		CLOGE("unlock with mtx failed, err: %d", errno); \
		exit(125); \
	}

#define CL_PURE_PRT   "\e[0m"

#define CL_RED_PRT    "\e[31m"
#define CL_GREEN_PRT  "\e[32m"
#define CL_YELLOW_PRT "\e[33m"
#define CL_BLUE_PRT   "\e[34m"
#define CL_PINK_PRT   "\e[35m"
#define CL_CYAN_PRT   "\e[36m"

#define CL_RED_BG_PRT    "\e[41m"
#define CL_GREEN_BG_PRT  "\e[42m"
#define CL_YELLOW_BG_PRT "\e[43m"
#define CL_BLUE_BG_PRT   "\e[44m"
#define CL_PINK_BG_PRT   "\e[45m"
#define CL_CYAN_BG_PRT   "\e[46m"

typedef struct {
	int len;
	uint8_t buf[64];
} Buf64;

typedef struct {
	int len;
	uint8_t buf[128];
} Buf128;

typedef struct {
	int len;
	uint8_t buf[256];
} Buf256;

typedef struct {
	int len;
	uint8_t buf[512];
} Buf512;

typedef struct {
	int len;
	uint8_t buf[768];
} Buf768;

typedef struct {
	int len;
	uint8_t buf[1024];
} Buf1k;

typedef struct {
	int len;
	uint8_t buf[1536];
} Buf1k5;

typedef struct {
	int len;
	uint8_t buf[2048];
} Buf2k;

typedef struct {
	int len;
	uint8_t buf[3072];
} Buf3k;

typedef struct {
	int len;
	uint8_t buf[4096];
} Buf4k;

typedef struct {
	int len;
	uint8_t buf[8192];
} Buf8k;

#endif

