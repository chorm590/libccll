#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "def.h"
#include "_def.h"
#include "_log.h"
#include "log.h"

TAG = TAG_PREFIX "mem";

Ret _get_running_mems(const char *process, int *vm_rss, int *vm_size, int *vm_peak, int *vm_data, int *vm_stk)
{
	typedef struct {
		long vmpeak;
		long vmsize;
		long vmrss;
		long vmdata;
		long vmstk;
		long vmexe;
		long vmlib;
	} MemInfo;

	FILE *fp;
	char buffer[256];
	int found_cnt = 0;
	const int item_cnt = sizeof(MemInfo) / sizeof(int);
	MemInfo mi = {0};

	{
		char proc_stt[64];
		sprintf(proc_stt, "/proc/%s/status", process);
		fp = fopen(proc_stt, "r");
	}
	if (fp == NULL) {
		CLOGE("get the info of memory failed, err: %d", errno);
		return FAIL;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if(found_cnt == item_cnt) break;
		found_cnt++;
		if (strncmp(buffer, "VmPeak:", 7) == 0) {
			sscanf(buffer + 7, "%ld", &mi.vmpeak);
		}
		else if (strncmp(buffer, "VmSize:", 7) == 0) {
			sscanf(buffer + 7, "%ld", &mi.vmsize);
		}
		else if (strncmp(buffer, "VmRSS:", 6) == 0) {
			sscanf(buffer + 6, "%ld", &mi.vmrss);
		}
		else if (strncmp(buffer, "VmData:", 7) == 0) {
			sscanf(buffer + 7, "%ld", &mi.vmdata);
		}
		else if (strncmp(buffer, "VmStk:", 6) == 0) {
			sscanf(buffer + 6, "%ld", &mi.vmstk);
		}
		else if (strncmp(buffer, "VmExe:", 6) == 0) {
			sscanf(buffer + 6, "%ld", &mi.vmexe);
		}
		else if (strncmp(buffer, "VmLib:", 6) == 0) {
			sscanf(buffer + 6, "%ld", &mi.vmlib);
		}
		else found_cnt--;
	}
	fclose(fp);

	*vm_rss = mi.vmrss;
	*vm_size = mi.vmsize;
	*vm_peak = mi.vmpeak;
	*vm_data = mi.vmdata;
	*vm_stk = mi.vmstk;

	return SUCC;
}

Ret cl_mem_get_curr_running_mems(int *vm_rss, int *vm_size, int *vm_peak, int *vm_data, int *vm_stk)
{
	return _get_running_mems("self", vm_rss, vm_size, vm_peak, vm_data, vm_stk);
}

Ret cl_mem_get_running_mems(pid_t pid, int *vm_rss, int *vm_size, int *vm_peak, int *vm_data, int *vm_stk)
{
	char pid_in_str[16];
	sprintf(pid_in_str, "%d", pid);
	return _get_running_mems(pid_in_str, vm_rss, vm_size, vm_peak, vm_data, vm_stk);
}

