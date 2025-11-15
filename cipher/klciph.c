#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "def.h"
#include "log.h"
#include "_klciph.h"
#include "klciph.h"

TAG = "klciph";
#undef TRACE
#define TRACE() ;

#define CH14S_SZ 16
static const uint16_t ch14s[] = { // characteristic table
	0x0827,
	0x8e87,
	0xc35d,
	0xedd3,

	0xcc88,
	0xb10a,
	0xcc02,
	0xd631,  

	0xbbc9,
	0x0194,
	0x1761,
	0x89d9,

   	0xb11f,
	0x36ee,
	0x1ad6,
	0x2148  
};

#define MAX_PLAIN_BYTES 100 
#define RANDOM_FP "/dev/random"

static Bool g_init = false;
static uint8_t cmdt[] = {255, 255, 255, 255, 255}; // y-m-d h:m


static Ret _cmdt_init()
{
	char cmpl_date[32];
	sprintf(cmpl_date, "%s", CMPLDATE);
	char *ptr = cmpl_date;
#define WORK(ch, idx) \
{ \
	char *pch = strchr(ptr, ch); \
	if(pch == NULL) \
	{ \
		CLOGE("[%d] invalid cmpl-date: %s", idx, cmpl_date); \
		memset(cmdt, 255, 5); \
		return FAIL; \
	} \
	*pch = 0; \
	cmdt[idx] = atoi(ptr); \
	ptr = pch + 1; \
}
	WORK('-', 0);
	cmdt[0] = cmdt[0] - 1900;
	WORK('-', 1);
	cmdt[1] = cmdt[1] - 1;
	WORK(' ', 2);
	WORK(':', 3);
	cmdt[4] = atoi(ptr);
#undef WORK

	return SUCC;
}

static Ret _chk_time()
{
	int i = 0;
	for(; i < 5; i++)
		if(cmdt[i] == 255) return FAIL;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	if(tm->tm_year < cmdt[0] ||
				tm->tm_mon < cmdt[1] ||
				tm->tm_mday < cmdt[2] ||
				tm->tm_hour < cmdt[3] ||
				tm->tm_min < cmdt[4])
		return FAIL;

	return SUCC;
}

Ret klciph_init()
{
	TRACE();
	if(g_init) return FAIL;

	if(_cmdt_init() != SUCC)
	{
		CLOGE("init compile date failed");
		return FAIL;
	}

	if(_chk_time() != SUCC)
	{
		CLOGE("Time invalid, klciph require a synchronous time-sys");
		return FAIL;
	}

	g_init = true;
	return SUCC;
}

void klciph_deinit()
{
	TRACE();
	g_init = false;
}

#define _CHK1() \
	if(!g_init) \
	{ \
		return KLCR_NOT_INIT; \
	}
#define _CHK2() \
	if(_chk_time() != SUCC) \
	{ \
		return KLCR_TIME_INVAL; \
	}
KLCP_RET klciph_enc(KLCIPH_MODE mode, uint8_t *plain, int plen, uint8_t *cipher, int *clen)
{
	TRACE();
	if(plain == NULL || cipher == NULL || clen == NULL) return KLCR_ERR;
	if(plen < 1 || plen > MAX_PLAIN_BYTES)
	{
		CLOGE("allowed plain len is: 1 ~ %d", MAX_PLAIN_BYTES);
		return KLCR_ERR;
	}
	_CHK1();
	_CHK2();

	FILE *frd;
	// Determine characteristic
	frd = fopen(RANDOM_FP, "r");
	if(frd == NULL)
	{
		CLOGE("ch14 init failed");
		return KLCR_ERR;
	}
	uint8_t ch14_idx;
	if(fread(&ch14_idx, 1, 1, frd) != 1)
	{
		CLOGE("ch14 init failed2");
		return KLCR_ERR;
	}
	ch14_idx %= CH14S_SZ;
	const uint16_t ch14 = ch14s[ch14_idx]; // Key concerpt: 1/3

	// Create the padding buffer
	// Check the padding buffer
	fclose(frd);

	// Determine the keys
	// Cipher
	// Fill it to buffer
	// Finish

	return KLCR_SUCC;
}

KLCP_RET klciph_dec(KLCIPH_MODE mode, uint8_t *cipher, int clen, uint8_t *plain, int *plen)
{
	TRACE();
	_CHK1();
	_CHK2();

	return KLCR_SUCC;
}

// Since 2025-11-15 05:40:58, klon at home.
