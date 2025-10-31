#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "def.h"
#include "_def.h"
#include "sh.h"
#include "_sh.h"

static void _read_out(FILE* pf)
{
	int cnt;
	int rlen;
	char buf[SH_READ_OUT_BUF_LEN];

	if(pf == NULL)
	{
		usc_log_info("null pf");
		return;
	}

	cnt = SH_READ_OUT_LOOP;
	while(cnt)
	{
		cnt--;
		rlen = fread(buf, 1, SH_READ_OUT_BUF_LEN, pf);
		if(rlen == SH_READ_OUT_BUF_LEN)
			continue;
		break;
	}
}

Ret usc_sh_exec(const char *cmd, char *result, int size, int *rlen)
{
	if(cmd == NULL || size < 0 || size > 0x100000)
	{
		usc_log_error("parameter invalid");
		return FAIL;
	}

	int cmd_len = strlen(cmd);
	char* cmd2 = (char*)calloc(1, cmd_len + 8);
	if(cmd2 == NULL)
	{
		usc_log_error("memory failed");
		return FAIL;
	}
	sprintf(cmd2, "%s 2>&1", cmd);

	FILE* pf = popen(cmd2, "r");
	free(cmd2);
	if(pf == NULL)
	{
		usc_log_error("exec cmd failed");
		return FAIL;
	}

	if(buf == NULL)
	{
		_read_out(pf);
		return WEXITSTATUS(pclose(pf)) ? FAIL : SUCC;
	}

	int len = fread(buf, 1, blen, pf);
	if(len < 0)
	{
		usc_log_error("read failed");
		return FAIL;
	}

	if(rlen)
	{
		*rlen = len;
	}

	if(len == blen)
	{
		_read_out(pf);
	}

	return WEXITSTATUS(pclose(pf)) ? FAIL : SUCC;
}
