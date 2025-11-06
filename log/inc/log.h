#ifndef __CL_LOG_H__
#define __CL_LOG_H__

typedef enum {
	DEBUG = 'D',
	INFO = 'I',
	WARN = 'W',
	ERROR = 'E'
} LogType;

void cl_log(LogType type, const char *tag, const char* msg, ...);

#define CL_TAG cltag // Each .c file that include this header must defined a char * variable named 'cltag'
				  // Eg: const static char *cltag = "my-tag";
#define TAG const static char *cltag

#define CLOGD(fmt, args...) cl_log(DEBUG, CL_TAG, fmt, ##args)
#define CLOGI(fmt, args...) cl_log(INFO, CL_TAG, fmt, ##args)
#define CLOGW(fmt, args...) cl_log(WARN, CL_TAG, fmt, ##args)
#define CLOGE(fmt, args...) cl_log(ERROR, CL_TAG, fmt, ##args)
#define TRACE() CLOGD("%s", __FUNCTION__)

#endif

