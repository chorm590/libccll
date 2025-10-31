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

static pthread_mutex_t lock;
static char initialized = false;

static inline void log_print(char* msg)
{
	if(print_timestamp)
	{
		time_t ti = {0};
		time(&ti);
		struct tm* t = localtime(&ti);
		struct timeval tv = {0};
		gettimeofday(&tv, NULL);
		printf("[%02d%02d%02d%02d%02d.%06ld] %s\n", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec, msg);
		if(rsyslogd.pid > 0 && rsyslogd.sys_log == uttrue)
		{
			int priority = 0;
			switch(*msg)
			{
				case 'D':
					priority = LOG_LOCAL0 | LOG_DEBUG;
					break;
				case 'I':
					priority = LOG_LOCAL0 | LOG_INFO;
					break;
				case 'E':
					priority = LOG_LOCAL0 | LOG_ERR;
					break;
				default:
					goto LOG_PRINT_END;
			}
			syslog(priority, "[%02d%02d%02d%02d%02d.%06ld] %s", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec, msg);
		}
	}
	else
	{
		printf("%s\n", msg);
		if(rsyslogd.pid > 0 && rsyslogd.sys_log == uttrue)
		{
			int priority = 0;
			switch(*msg)
			{
				case 'D':
					priority = LOG_LOCAL0 | LOG_DEBUG;
					break;
				case 'I':
					priority = LOG_LOCAL0 | LOG_INFO;
					break;
				case 'E':
					priority = LOG_LOCAL0 | LOG_ERR;
					break;
				default:
					goto LOG_PRINT_END;
			}
			syslog(priority, "%s", msg);
		}
	}

LOG_PRINT_END:
	return;
}

void log_(LogType type, const char *tag, const char* msg, ...)
{
	va_list args;
	va_start(args, pked_buff);
	vsnprintf(upkbf, LOG_BUF_LEN, pked_buff, args);
	va_end(args);
		char buf[LOG_BUF_LEN] = {'D', ' ', 0};
		UNPACK_MSG(msg, buf + 2);
		if(strlen(buf) < (LOG_BUF_LEN - 1))
		{
			log_print(buf);
			ret = SUCCESS;
		}
}

Ret log_init()
{
	Ret ret = SUCC;
	if(pthread_mutex_init(&lock, NULL))
	{
		perror("mutex_init");
		ret = FAIL;
	}

	initialized = true;
	return ret;
}

void log_deinit()
{
	initialized = false;
	pthread_mutex_destroy(&lock);
}

