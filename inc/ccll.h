#ifndef __CCLL_H__
#define __CCLL_H__

typedef int (*printf_fun)(const char *format, ...);

Ret cl_init(print_fun pfun);

#endif
