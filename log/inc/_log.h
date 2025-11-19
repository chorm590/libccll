#ifndef __CL__LOG_H__
#define __CL__LOG_H__

#define LOG_HDR_SZ 128
#define LOG_BUF_SZ 8192

Ret cl_log_init();
void cl_log_deinit();
#ifndef __CCLL_H__
typedef int (*print_fun)(int type, const char *tag, const char *text);
#endif
print_fun cl_log_get_def_prtfun();

#define LOG_INIT_FAIL(module) \
	CLOGE("init the " #module " failed, err: %d", errno)

#define LOG_LOCK_FAIL(module) \
	CLOGE("lock for the " #module " failed, err: %d", errno)

#define LOG_UNLOCK_FAIL(module) \
	CLOGE("unlock for the " #module " failed, err: %d", errno)

#define TAG_PREFIX "libcl_"

#endif

