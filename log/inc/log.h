#ifndef __CL_LOG_H__
#define __CL_LOG_H__

typedef enum {
	DEBUG,
	INFO,
	WARN,
	ERROR
} LogType;

typedef Ret (*log_printer)(int type, const char *tag, const char *text);

void log_(LogType type, const char *tag, const char* msg, ...);
Ret log_init();
void log_deinit();
Ret register_log_printer(log_printer printer);


#endif

