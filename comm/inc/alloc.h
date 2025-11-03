#ifndef __CL_ALLOC_H__
#define __CL_ALLOC_H__

void * cl_malloc(const char *caller_fun_name, const int caller_line_no, const char *tag, int size_on_bytes);
void cl_free(void *ptr);
#define MALLOC(bytes) cl_malloc(__FUNCTION__, __LINE__, cltag, bytes)
#define FREE(ptr) cl_free(ptr)
void cl_iter_objs();

#endif
