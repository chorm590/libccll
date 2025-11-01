#ifndef __CCLL_H__
#define __CCLL_H__

typedef int (*print_fun)(int type, const char *tag, const char *text);

/*
 * @param pfun [in]
 *        The log-print function. if NULL, the default(printf in stdio.h) will be selected.
 * */
Ret cl_init(print_fun pfun);
void cl_deinit();

#endif
