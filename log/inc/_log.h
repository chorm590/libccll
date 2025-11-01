#ifndef __CL__LOG_H__
#define __CL__LOG_H__

#define LOG_HDR_SZ 128
#define LOG_BUF_SZ 8192

Ret log_init();
void log_deinit();
print_fun log_get_def_prtfun();

#endif

