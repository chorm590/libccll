#ifndef __CCLL_H__
#define __CCLL_H__

#ifndef __CL_LOG_TYPE__
#define __CL_LOG_TYPE__
typedef enum {
	CL_DEBUG = 'D',
	CL_INFO = 'I',
	CL_WARN = 'W',
	CL_ERROR = 'E'
} CL_LogType;
#endif
typedef int (*print_fun)(CL_LogType type, const char *tag, const char *text);

/*
 * @param pfun [in]
 *        The log-print function. if NULL, the default(printf in stdio.h) will be selected.
 * */
Ret cl_init(print_fun pfun);
void cl_deinit();

#endif
