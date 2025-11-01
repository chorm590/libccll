#ifndef __CL__DEF_H__
#define __CL__DEF_H__

#include <stddef.h>

#define NP     "NULL param"
#define NM     "no memory"

#define container_of(ptr, type, member) ({ \
		const typeof(((type *) 0)->member) *__mptr = (ptr); \
		(type *) ((char *) __mptr - offsetof(type, member)); \
		})

#endif
