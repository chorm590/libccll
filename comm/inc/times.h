#ifndef __CL_TIMES_H__
#define __CL_TIMES_H__

/*
 * '2025-11-25 21:44:19' --> 1764078308
 * */
Ret cl_time_date2sec1(const char *time_str, time_t *output);

/*
 * 1764078308 -> '2025-11-25 21:44:19'
 * */
void cl_time_sec2date1(const time_t sec, char *time_str);

#endif
