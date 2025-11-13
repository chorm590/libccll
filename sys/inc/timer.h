#ifndef __CL_TIMER_H__
#define __CL_TIMER_H__

/*
 * The callback of timeout.
 * Don't do too much work here.
 * */
typedef void (*cl_timer_cb)();

/*
 * Start a new timer.
 * 
 * @param sec
 * 		  The seconds period
 * @param ms
 * 		  The milliseconds period
 * @param repeats
 * 		  The repeat times, if zero, will repeat forever.
 * @param cb
 * 		  The callback function. A cb-fun can only be register once at a time.
 * */
Ret cl_timer_set(const uint16_t sec, const uint16_t ms, const int repeats, cl_timer_cb cb);

Ret cl_timer_cancel(cl_timer_cb cb);

int cl_timer_count();

#endif
