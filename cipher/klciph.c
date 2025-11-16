#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "def.h"
#include "log.h"
#include "_klciph.h"
#include "klciph.h"
#include "bytes.h"
#include "alloc.h"

TAG = "klciph";
#undef TRACE
#define TRACE() ;

#define DBG 1 // Print the running log

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

#define MAX_PLAIN_BYTES 120 // data 120-bytes
							// info 40-bytes about
							// 160 in total for msg
							// 90+ bytes can be use for confussing
#define RANDOM_FP "/dev/random"

static Bool g_init = false;


Ret klciph_init()
{
	TRACE();
	if(g_init) return FAIL;

	g_init = true;
	return SUCC;
}

void klciph_deinit()
{
	TRACE();
	g_init = false;
}



#define _CHK() \
	if(!g_init) \
	{ \
		CLOGE("hadn't init"); \
		return FAIL; \
	}
Ret klciph_enc(uint8_t *plain, int plen, uint8_t *cipher, int *clen)
{
	TRACE();
	_CHK();
	if(plain == NULL || cipher == NULL || clen == NULL) return FAIL;
	if(plen < 1 || plen > MAX_PLAIN_BYTES) return FAIL;

	FILE *frd;
	// Determine characteristic
	frd = fopen(RANDOM_FP, "r");
	if(frd == NULL)
	{
		CLOGE("init failed");
		return FAIL;
	}
	uint8_t ch14_idx;
	if(fread(&ch14_idx, 1, 1, frd) != 1)
	{
		CLOGE("init k1 failed");
		goto ERR_OCR_RET4633;
	}
	ch14_idx %= CH14S_SZ; // Key concerpt: 1/3, the characteristic
#if DBG
	CLOGD("idx: %d, ch14: 0x%04x", ch14_idx, ch14s[ch14_idx]);
#endif

	// Create the padding buffer
	uint8_t buffer[256]; // Key concerpt: 2/3, the padding buffer
	if(fread(buffer, 1, 256, frd) != 256)
	{
		CLOGE("buffer init failed");
		goto ERR_OCR_RET4633;
	}
#if DBG
	cl_print_bytes(buffer, 256);
#endif

	// Pre-set the padding buffer
	int ch_idx = -1;
{
	// insert the characteristic
	int i, j;
	int x;
	for(i = 0, x = -1; i < 200; i++)
	{
		const uint16_t a = *((uint16_t *) (buffer + i));
		if(a == ch14s[ch14_idx])
		{
#if DBG
			CLOGD("found ch14, idx: %d", i);
#endif
			x = i;
			break;
		}
	}
	if(x == -1) // Not found ch14, create it.
	{
#if DBG
		CLOGD("not found the ch14, create it.");
#endif
		for(i = 0; i < 200; i++)
		{
			if(buffer[i] < 200)
			{
				ch_idx = buffer[i];
				*((uint16_t *) (buffer + ch_idx)) = ch14s[ch14_idx];
#if DBG
				CLOGD("pos found: %d, ch14: 0x%04x", ch_idx, ch14s[ch14_idx]);
				int k;
				for(j = 0, k = 0; j < 256; j++)
				{
					if(j == ch_idx) printf("\e[41m");
					else if(j == (ch_idx + 2)) printf("\e[0m");

					if(k == 15)
					{
						k = 0;
						printf("%02x\n", buffer[j]);
					}
					else
					{
						k++;
						printf("%02x", buffer[j]);
					}
				}
				printf("\n");
#endif
				break;
			}
		}
	}
}

	// Determine the keys
	uint8_t key1, key2;
	int i_idx = -1; // The index of index.
{
	// get the key and set the len
	if(ch_idx == -1) return FAIL;
	key1 = buffer[ch_idx + 2];
	buffer[ch_idx + 3] = ch_idx;
	buffer[ch_idx + 4] = (uint8_t) plen;
	buffer[ch_idx + 5] = ch_idx + 6;
	key2 = buffer[ch_idx + 6];
	i_idx = ch_idx + 7;
#if DBG
	CLOGD("The key1: 0x%02x, key2: 0x%02x", key1, key2);
	int i, j;
	for(i = 0, j = 0; i < 256; i++)
	{
		if(i == (ch_idx + 2)) printf("\e[41m");
		else if(i == (ch_idx + 7)) printf("\e[0m");

		if(j == 15)
		{
			j = 0;
			printf("%02x\n", buffer[i]);
		}
		else
		{
			j++;
			printf("%02x", buffer[i]);
		}
	}
	printf("\n");
#endif
}

	int i, j;
	// Encrypt
	// encrypt the len
	buffer[ch_idx + 4] ^= key1;
#if DBG
	CLOGD("encrypt the len");
	for(i = 0, j = 0; i < 256; i++)
	{
		if(i == (ch_idx + 4)) printf("\e[41m");
		else if(i == (ch_idx + 5)) printf("\e[0m");

		if(j == 15)
		{
			j = 0;
			printf("%02x\n", buffer[i]);
		}
		else
		{
			j++;
			printf("%02x", buffer[i]);
		}
	}
	printf("\n");
#endif

	// check and pre-set the index
	// each index indicate 4B data
	const int idx_amt = (int) ceil((double) plen / 4.0);
#if DBG
	CLOGD("amt of idx: %d", idx_amt);
#endif
	for(i = i_idx; i < idx_amt; i++)
	{
		correct the indices
	}

	fclose(frd);

	// Fill it to buffer
	// Finish

	return SUCC;

ERR_OCR_RET4633:
	fclose(frd);
	return FAIL;
}

Ret klciph_dec(uint8_t *cipher, int clen, uint8_t *plain, int *plen)
{
	TRACE();
	_CHK();

	return SUCC;
}

// Since 2025-11-15 05:40:58, klon at home.
