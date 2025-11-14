#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "def.h"
#include "log.h"
#include "ini.h"
#include "txt.h"

#define MAX_KLEN 64
#define MAX_SLEN MAX_KLEN
#define MAX_LINE_BYTE 512

TAG = "lw-ini";

typedef enum {
	FR_FOUND,
	FR_NONE,
	FR_FAIL
} FindRet;

static FindRet _get_value(char *line, const char *sec, char *_sec, const char *key, char *value)
{
	if(line == NULL || strlen(line) < 1) return FR_FAIL;

	line[strcspn(line, "\n")] = '\0';
	const size_t llen = strlen(line);

	if (line[0] == '\0' || line[0] == ';' || line[0] == '#') return FR_NONE;

	if ((line[0] == '[') && (line[llen - 1] == ']'))
	{
		memcpy(_sec, line + 1, llen - 2);
		_sec[llen - 2] = 0;
		if(sec == NULL)
		{
			CLOGE("sec found but require no-sec");
			return FR_FAIL;
		}
		return FR_NONE;
	}

	if(sec && strcmp(sec, _sec)) return FR_NONE;

	char *delimiter = strchr(line, '=');
	if (delimiter == NULL) return FR_NONE;

	*delimiter = '\0';
	char *_key = line;
	char *_val = delimiter + 1;

	while (*_key == ' ' || *_key == '\t') _key++;
	char *end = delimiter - 1;
	while (end > _key && (*end == ' ' || *end == '\t')) end--;
	*(end + 1) = '\0';
	if(strcmp(_key, key)) return FR_NONE;

	while (*_val == ' ' || *_val == '\t') _val++;
	end = &line[llen - 1];
	while (end > _val && (*end == ' ' || *end == '\t')) end--;
	*(end + 1) = '\0';

	memcpy(value, _val, end - _val + 2); // included the '\0'

	return FR_FOUND;
}

Ret cl_ini_get(const char *fn, const char *section, const char *key, char *value)
{
	TRACE();

	if(fn == NULL || key == NULL || value == NULL) return FAIL;

	{
		const size_t klen = strlen(key);
		if(klen < 1 || klen > MAX_KLEN)
		{
			CLOGE("len exceed in key");
			return FAIL;
		}

		if(section)
		{
			const size_t slen = strlen(section);
			if(slen <  1 || slen > MAX_SLEN)
			{
				CLOGE("len exceed in section");
				return FAIL;
			}
		}
	}

    FILE *file = fopen(fn, "r");
    if (file == NULL)
	{
		CLOGE("open %s failed, err: %d", fn, errno);
		return FAIL;
	}

	char found = 0;
    char line[MAX_LINE_BYTE];
	char _sec[MAX_SLEN] = {0};

    while (fgets(line, MAX_LINE_BYTE, file) != NULL)
	{
		switch(_get_value(line, section, _sec, key, value))
		{
			case FR_FOUND:
				found = 1;
				goto OUTWHILE5738;
			case FR_NONE:
				break;
			default:
				goto OUTWHILE5738;
		}
    }
OUTWHILE5738:;

    fclose(file);

    return (found ? SUCC : FAIL);
}

Ret cl_ini_get2(char *cfg_txt, const char *section, const char *key, char *value)
{
	TRACE();

	if(cfg_txt == NULL || key == NULL || value == NULL) return FAIL;

	{
		const size_t klen = strlen(key);
		if(klen < 1 || klen > MAX_KLEN)
		{
			CLOGE("len exceed in key");
			return FAIL;
		}

		if(section)
		{
			const size_t slen = strlen(section);
			if(slen <  1 || slen > MAX_SLEN)
			{
				CLOGE("len exceed in section");
				return FAIL;
			}
		}
	}

	const size_t tlen = strlen(cfg_txt);
#define MAX_TXT_BYTE 1048576 // 1M
	if(tlen < 1 || tlen > MAX_TXT_BYTE) return FAIL;

	char found = 0;
    char line[MAX_LINE_BYTE];
	char _sec[MAX_SLEN] = {0};
	char *txt = cfg_txt;
	int off;

    while ((off = cl_txt_pos_line(line, MAX_LINE_BYTE, txt)) != -1)
	{
		switch(_get_value(line, section, _sec, key, value))
		{
			case FR_FOUND:
				found = 1;
				goto OUTWHILE1324;
			case FR_NONE:
				break;
			default:
				goto OUTWHILE1324;
		}
		if(off == 0) break;
		txt += off;
    }
OUTWHILE1324:;

    return (found ? SUCC : FAIL);
}

