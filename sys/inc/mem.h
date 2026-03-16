#ifndef __CL_MEM_H__
#define __CL_MEM_H__

/*
 * Get the memory info of current app.
 * Unit: KB
 * */
Ret cl_mem_get_curr_running_mems(int *vm_rss, int *vm_size, int *vm_peak, int *vm_data, int *vm_stk);

/*
 * Get the memory info of specify process.
 * Unit: KB
 * */
Ret cl_mem_get_running_mems(pid_t pid, int *vm_rss, int *vm_size, int *vm_peak, int *vm_data, int *vm_stk);

#endif
