#ifndef __CL_LOG_H__
#define __CL_LOG_H__

void cl_log(CL_LogType type, const char *tag, const char* msg, ...);

#define CL_TAG cltag // Each .c file that include this header must defined a char * variable named 'cltag'
					 // Eg: const static char *cltag = "my-tag";
#define TAG const static char *cltag

#define CLOGD(fmt, args...) cl_log(CL_DEBUG, CL_TAG, fmt, ##args)
#define CLOGI(fmt, args...) cl_log(CL_INFO, CL_TAG, fmt, ##args)
#define CLOGW(fmt, args...) cl_log(CL_WARN, CL_TAG, fmt, ##args)
#define CLOGE(fmt, args...) cl_log(CL_ERROR, CL_TAG, fmt, ##args)
#define TRACE() CLOGI("-> %s", __FUNCTION__)



#ifdef DISABLE_TRACE
#undef TRACE
#define TRACE() ;
#endif

#ifdef DISABLE_CLOGD
#undef CLOGD
#define CLOGD(fmt, args...) ;
#endif

#endif

