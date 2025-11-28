#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "def.h"
#include "times.h"

Ret cl_time_date2sec1(const char *time_str, time_t *output)
{
	size_t len = strlen(time_str);
	if(len != 19) // expected format: 2025-10-13 09:19:09
		return FAIL;

	char tbuf[20];
	strcpy(tbuf, time_str);

	int ey, emo, ed, eh, emi, es;
	// e-year
	char *a = tbuf;
	char *b = strchr(a, '-');
	if(b == NULL)
		return FAIL;
	*b = 0;
	ey = atoi(a);
	if(ey < 1970 || ey > 2099)
		return FAIL;
	a = b + 1;
	// e-month
	b = strchr(a, '-');
	if(b == NULL)
		return FAIL;
	*b = 0;
	emo = atoi(a);
	if(emo < 1 || emo > 12)
		return FAIL;
	a = b + 1;
	// e-day
	b = strchr(a, ' ');
	if(b == NULL)
		return FAIL;
	*b = 0;
	ed = atoi(a);
	if(ed < 1 || ed > 31)
		return FAIL;
	a = b + 1;
	// e-hour
	b = strchr(a, ':');
	if(b == NULL)
		return FAIL;
	*b = 0;
	eh = atoi(a);
	if(eh < 0 || eh > 23)
		return FAIL;
	a = b + 1;
	// e-minuts
	b = strchr(a, ':');
	if(b == NULL)
		return FAIL;
	*b = 0;
	emi = atoi(a);
	if(emi < 0 || emi > 59)
		return FAIL;
	a = b + 1;
	// e-second
	es = atoi(a);
	if(es < 0 || es > 59)
		return FAIL;

	struct tm from = {
		.tm_year = ey - 1900,
		.tm_mon = emo - 1,
		.tm_mday = ed,
		.tm_hour = eh,
		.tm_min = emi,
		.tm_sec = es
	};
	*output = mktime(&from);

	return *output == (time_t) -1 ? FAIL : SUCC;
}

void cl_time_sec2date1(const time_t sec, char *time_str)
{
	if(sec < 0 || time_str == NULL) return;

	struct tm *a = localtime(&sec);
	strftime(time_str, 20, "%F %T", a);
}
