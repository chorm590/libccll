#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "def.h"
#include "_def.h"
#include "sh.h"
#include "log.h"
#include "alloc.h"

TAG = "sh";


static void _read_all_out(FILE* pf)
{
#define SH_READ_OUT_BUF_LEN 2048
	char buf[SH_READ_OUT_BUF_LEN];

	if(pf == NULL)
	{
		CLOGE("null pf");
		return;
	}

	int cnt = 10000;
	while(cnt--)
	{
		if(fread(buf, 1, SH_READ_OUT_BUF_LEN, pf) < SH_READ_OUT_BUF_LEN)
			break;
	}
#undef SH_READ_OUT_BUF_LEN
}

Ret cl_sh_exec(const char *cmd, char *result, int size, int *rlen)
{
	if(cmd == NULL || size < 0 || size > 0x100000)
	{
		CLOGE("parameter invalid");
		return FAIL;
	}

	char *cmd_exe = (char *) MALLOC(strlen(cmd) + 8);
	sprintf(cmd_exe, "%s 2>&1", cmd);

	FILE* pf = popen(cmd_exe, "r");
	FREE(cmd_exe);
	if(pf == NULL)
	{
		CLOGE("cmd exec failed");
		return FAIL;
	}

	if(result == NULL)
	{
		_read_all_out(pf);
		return WEXITSTATUS(pclose(pf)) ? FAIL : SUCC;
	}

	const int rsz = size - 1;
	if(rsz < 0)
	{
		_read_all_out(pf);
		return WEXITSTATUS(pclose(pf)) ? FAIL : SUCC;
	}

	const int len = fread(result, 1, rsz, pf);
	if(len < 0)
	{
		CLOGE("read cmd result failed");
		return FAIL;
	}
	*(result + rsz) = 0;

	if(rlen)
	{
		*rlen = len;
	}

	if(len == rsz)
	{
		_read_all_out(pf);
	}

	return WEXITSTATUS(pclose(pf)) ? FAIL : SUCC;
}

