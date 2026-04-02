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
#include "log_type.h"
#include "_log.h"
#include "log.h"
#include "wait.h"

extern print_fun s_prtfun;

static char g_log_hdr[128];
static char *g_log_buf;
static pthread_mutex_t g_lock;
static char g_init = false;

static int def_print_fun(int type, const char *tag, const char *text)
{
	if(!g_init) return FAIL;

	struct timeval a;
	struct tm *b;
	gettimeofday(&a, NULL);
	b = localtime(&a.tv_sec);
	strftime(g_log_hdr, 30, "%F %T", b); // automatically append the terminate-character at the end.
										 // Format: 2025-11-01 12:49:38
	sprintf(g_log_hdr + 19, ".%03d", (int) (a.tv_usec >> 10));
	CL_LogType lt = (CL_LogType) type;
	sprintf(g_log_hdr + 23, " %C-%s: ", lt, tag);

	switch(type) {
		case CL_DEBUG:
			printf(CL_WHITE_BG_PRT CL_BLK_PRT " %s%s " CL_PURE_PRT "\n", g_log_hdr, text);
			break;
		case CL_INFO:
			printf("%s%s\n", g_log_hdr, text);
			break;
		case CL_WARN:
			printf(CL_YELLOW_PRT "%s%s\n" CL_PURE_PRT, g_log_hdr, text);
			break;
		case CL_ERROR:
			printf(CL_RED_PRT "%s%s\n" CL_PURE_PRT, g_log_hdr, text);
			break;
		default:
			printf("%s%s\n", g_log_hdr, text);
			break;
	}

	return SUCC;
}

void cl_log(CL_LogType type, const char *tag, const char* msg, ...)
{
	if(pthread_mutex_lock(&g_lock))
	{
		fprintf(stderr, "lock for log failed, err: %d", errno);
		exit(124);
	}

	va_list args;
	va_start(args, msg);
	vsnprintf(g_log_buf, LOG_BUF_SZ - 1, msg, args);
	va_end(args);

	if(s_prtfun) s_prtfun(type, tag, g_log_buf);
	else def_print_fun(type, tag, g_log_buf);

	if(pthread_mutex_unlock(&g_lock))
	{
		fprintf(stderr, "unlock for log failed, err: %d", errno);
		exit(125);
	}
}

print_fun cl_log_get_def_prtfun()
{
	return def_print_fun;
}

Ret cl_log_init()
{
	if(g_init) return SUCC;

	if(pthread_mutex_init(&g_lock, NULL))
	{
		perror("mutex_init");
		return FAIL;
	}

	g_log_buf = (char *) malloc(LOG_BUF_SZ);
	if(g_log_buf == NULL)
	{
		perror("malloc log-buf");
		return FAIL;
	}

	g_init = true;
	return SUCC;
}

void cl_log_deinit()
{
	g_init = false;
	SLEEP_MS(50); // To wait log-print finish
	free(g_log_buf);
	g_log_buf = NULL;
	pthread_mutex_destroy(&g_lock);
}

