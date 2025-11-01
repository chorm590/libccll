#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#include "def.h"
#include "ccll.h"
#include "_log.h"
#include "log.h"

extern print_fun s_prtfun;

static char *log_hdr;
static char *log_buf;
static pthread_mutex_t lock;
static char g_init = false;

static int def_print_fun(int type, const char *tag, const char *text)
{
	if(!g_init) return FAIL;

	struct timeval a;
	struct tm *b;
	gettimeofday(&a, NULL);
	b = localtime(&a.tv_sec);
	strftime(log_hdr, 30, "%F %T", b); // automatically append the terminate-character at the end.
									   // Format: 2025-11-01 12:49:38
	sprintf(log_hdr + 19, ".%03d", (int) (a.tv_usec >> 10));
	LogType lt;
	switch(type)
	{
		case 1:
			lt = INFO;
			break;
		case 2:
			lt = WARN;
			break;
		case 3:
			lt = ERROR;
			break;
		default:
			lt = DEBUG;
			break;
	}
	sprintf(log_hdr + 23, " %C-%s: ", lt, tag);

	printf("%s%s\n", log_hdr, text);

	return SUCC;
}

void cl_log(LogType type, const char *tag, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf(log_buf, LOG_BUF_SZ - 1, msg, args);
	va_end(args);

	if(s_prtfun) s_prtfun(type, tag, log_buf);
	else def_print_fun(type, tag, log_buf);
}

print_fun log_get_def_prtfun()
{
	return def_print_fun;
}

Ret log_init()
{
	if(g_init) return SUCC;
	if(pthread_mutex_init(&lock, NULL))
	{
		perror("mutex_init");
		return FAIL;
	}

	log_hdr = (char *) malloc(LOG_HDR_SZ);
	if(log_hdr == NULL)
	{
		perror("malloc log-hdr");
		return FAIL;
	}

	log_buf = (char *) malloc(LOG_BUF_SZ);
	if(log_buf == NULL)
	{
		perror("malloc log-buf");
		free(log_hdr);
		log_hdr = NULL;
		return FAIL;
	}

	g_init = true;
	return SUCC;
}

void log_deinit()
{
	g_init = false;
	{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = 50 * 1000000 // 50ms
	};
	nanosleep(&ts, NULL); // To wait log-print finish
	}
	free(log_hdr);
	free(log_buf);
	log_hdr = NULL;
	log_buf = NULL;
	pthread_mutex_destroy(&lock);
}

