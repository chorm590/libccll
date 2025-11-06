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

static char *l_log_hdr;
static char *l_log_buf;
static pthread_mutex_t l_lock;
static char g_init = false;

static int def_print_fun(int type, const char *tag, const char *text)
{
	if(!g_init) return FAIL;

	struct timeval a;
	struct tm *b;
	gettimeofday(&a, NULL);
	b = localtime(&a.tv_sec);
	strftime(l_log_hdr, 30, "%F %T", b); // automatically append the terminate-character at the end.
										 // Format: 2025-11-01 12:49:38
	sprintf(l_log_hdr + 19, ".%03d", (int) (a.tv_usec >> 10));
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
	sprintf(l_log_hdr + 23, " %C-%s: ", lt, tag);

	printf("%s%s\n", l_log_hdr, text);

	return SUCC;
}

void cl_log(LogType type, const char *tag, const char* msg, ...)
{
	if(pthread_mutex_lock(&l_lock))
	{
		fprintf(stderr, "lock for log failed\n");
		return;
	}

	va_list args;
	va_start(args, msg);
	vsnprintf(l_log_buf, LOG_BUF_SZ - 1, msg, args);
	va_end(args);

	if(s_prtfun) s_prtfun(type, tag, l_log_buf);
	else def_print_fun(type, tag, l_log_buf);
	pthread_mutex_unlock(&l_lock);
}

print_fun cl_log_get_def_prtfun()
{
	return def_print_fun;
}

Ret cl_log_init()
{
	if(g_init) return SUCC;
	if(pthread_mutex_init(&l_lock, NULL))
	{
		perror("mutex_init");
		return FAIL;
	}

	l_log_hdr = (char *) malloc(LOG_HDR_SZ);
	if(l_log_hdr == NULL)
	{
		perror("malloc log-hdr");
		return FAIL;
	}

	l_log_buf = (char *) malloc(LOG_BUF_SZ);
	if(l_log_buf == NULL)
	{
		perror("malloc log-buf");
		free(l_log_hdr);
		l_log_hdr = NULL;
		return FAIL;
	}

	g_init = true;
	return SUCC;
}

void cl_log_deinit()
{
	g_init = false;
	{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = 50 * 1000000 // 50ms
	};
	nanosleep(&ts, NULL); // To wait log-print finish
	}
	free(l_log_hdr);
	free(l_log_buf);
	l_log_hdr = NULL;
	l_log_buf = NULL;
	pthread_mutex_destroy(&l_lock);
}

