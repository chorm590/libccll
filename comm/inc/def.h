#ifndef __CL_DEF_H__
#define __CL_DEF_H__

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

#endif

