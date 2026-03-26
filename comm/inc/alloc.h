#ifndef __CL_ALLOC_H__
#define __CL_ALLOC_H__

void * cl_malloc(const char *caller_fun_name, const int caller_line_no, const char *tag, int size_on_bytes);
void cl_free(void *ptr);
#define MALLOC(bytes) cl_malloc(__FUNCTION__, __LINE__, cltag, bytes)
#define FREE(ptr) cl_free(ptr)
uint32_t cl_allocing_cnt();
size_t cl_allocing_bytes();

/*
 * print all the allocating info to 'buf'.
 * @buf [out]
 *      you must allocate by 'malloc' in stdlib.h by yourself.
 *      also, you must release it by yourself.
 *      you could get the count of allocating by calling 'cl_allocing_cnt'
 *      before calling the 'malloc', and each allocing info would take 96 bytes to print.
 * */
void cl_alloc_get_stat(char *buf);

#endif
