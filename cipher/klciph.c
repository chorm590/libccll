#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "def.h"
#include "log_type.h"
#include "_log.h"
#define DISABLE_TRACE
#include "log.h"
#include "klciph.h"
#include "convt.h"
#include "alloc.h"

TAG = TAG_PREFIX "klciph";
#define DISABLE_TRACE

#define DBG 0 // Print the running log

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
#define BEG_IDX 97


static int _get_char_idx(FILE *frd)
{
	uint8_t idx;
	if(fread(&idx, 1, 1, frd) != 1)
	{
		CLOGE("failed2");
		return -1;
	}

	return (idx % CH14S_SZ);
}

static int _get_begin_idx(FILE *frd)
{
	uint8_t idx;
	int cnt = 0;

AGAIN4214:
	if(cnt++ > 200)
	{
		CLOGE("failed4");
		return -1;
	}

	if(fread(&idx, 1, 1, frd) != 1)
	{
		CLOGE("failed5");
		return -1;
	}

	if(idx == BEG_IDX) goto AGAIN4214;

	return idx;
}

static Ret _init_buffer(uint8_t *bf256, FILE *frd)
{
	if(fread(bf256, 1, 256, frd) != 256)
	{
		CLOGE("failed4");
		return FAIL;
	}

	return SUCC;
}

static int _get_free_pos(uint8_t *bucket, int from)
{
	int i;
	for(i = 0; i < 256; i++)
	{
		from++;
		if(from == 256) from = 0;
		if(*(bucket + from)) continue;
		return from;
	}

	return -1;
}
#define GFROM() \
{ \
	from = _get_free_pos(bucket, from); \
	if(from == -1) \
	{ \
		CLOGE("gen failed1"); \
		return FAIL; \
	} \
}

static int _get_data_idx(uint8_t *bucket, FILE *frd)
{
	uint8_t idx;
	int cnt = 0;

AGAIN3439:
	if(cnt++ > 220)
	{
		CLOGE("failed4054");
		return -1;
	}

	if(fread(&idx, 1, 1, frd) != 1)
	{
		CLOGE("failed7405");
		return -1;
	}
	int idx2 = idx;

	if(*(bucket + idx2++)) goto AGAIN3439;
	if(idx2 == 256) idx2 = 0;
	if(*(bucket + idx2++)) goto AGAIN3439;
	if(idx2 == 256) idx2 = 0;
	if(*(bucket + idx2++)) goto AGAIN3439;
	if(idx2 == 256) idx2 = 0;
	if(*(bucket + idx2++)) goto AGAIN3439;

	return idx;
}

static void _put_data(const uint8_t *data, const int len, const int from, const int idx, const uint8_t key1, const uint8_t key2, uint8_t *bucket, uint8_t *buffer)
{
	const bool parity = from & 1;
#if DBG
	int a = -1, b = -1, c = -1, d = -1;
#endif

	int idx2 = idx;
	*(buffer + idx2) = parity ? *(data + 0) ^ key2 : *(data + 0) ^ key1;
	*(bucket + idx2) = 1;
#if DBG
	a = idx2;
#endif
	idx2++;
	if(len == 1) goto PUT_END1407;

	if(idx2 == 256) idx2 = 0;
	*(buffer + idx2) = parity ? *(data + 1) ^ key2 : *(data + 1) ^ key1;
	*(bucket + idx2) = 1;
#if DBG
	b = idx2;
#endif
	idx2++;
	if(len == 2) goto PUT_END1407;

	if(idx2 == 256) idx2 = 0;
	*(buffer + idx2) = parity ? *(data + 2) ^ key2 : *(data + 2) ^ key1;
	*(bucket + idx2) = 1;
#if DBG
	c = idx2;
#endif
	idx2++;
	if(len == 3) goto PUT_END1407;

	if(idx2 == 256) idx2 = 0;
	*(buffer + idx2) = parity ? *(data + 3) ^ key2 : *(data + 3) ^ key1;
	*(bucket + idx2) = 1;
#if DBG
	d = idx2;
#endif
	idx2++;

PUT_END1407:;

#if DBG
{
	CLOGD("--> Put and encrypt the index and data");
	int j, k;
	for(j = 0, k = 0; j < 256; j++)
	{
		if(j == from || j == a || j == b || j == c || j == d) printf("\e[41m");
		else printf("\e[0m");

		printf("%02x", buffer[j]);
		if(k++ == 15)
		{
			k = 0;
			printf("\e[0m\n");
		}
	}
	printf("\e[0m\n");
}
#endif
}

Ret cl_klciph_enc(uint8_t *plain, int plen, uint8_t *cipher, int *clen)
{
	TRACE();
	if(plain == NULL || cipher == NULL || clen == NULL) return FAIL;
	if(plen < 1 || plen > MAX_PLAIN_BYTES) return FAIL;

/*

   The rule:

	da7 da8 002 003 004 005 006 007 008 009 010 011 012 013 014 015
	016 017 018 019 020 021 022 023 024 025 026 027 028 029 030 031
	032 033 034 035 036 037 038 039 040 041 042 043 044 045 046 047
	048 049 050 051 052 053 054 da9 056 057 058 059 060 061 062 063
	064 065 066 067 068 069 070 071 072 073 074 075 076 077 078 079
	080 081 082 083 084 085 086 087 088 089 090 091 092 093 094 095
	096 beg 098 099 100 101 102 103 104 105 106 107 108 109 110 111
	112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127
	128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143
	144 145 146 147 148 149 150 ch2 ch1  k1 154 len 156  k2 id1 id2
	id3   m   d   h   s 165 166 167 168 169 170 171 172 173 174 175
	176 da1 da2 da3 da4 181 182 183 184 185 186 187 188 189 190 191
	192 193 194 195 196 197 198 199 200 201 202 203 204 205 206 207
	208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223
	224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 239
	240 241 242 243 244 245 246 247 248 249 250 251 252 253 da5 da6

	 1. Find a random pos to storage the characteristic
	 2. Put the pos to buf[97]
	 3. Put the characteristic(2 bytes) to the pos
	 4. Get the key1
	 5. Put the len
	 6. Get the key2
	 7. Encrypt the characteristic[1] with key2 and len with key1
	 8. Put the index sequence and data
	 9. Odd idx encrypt with k1
	10. Even idx encrypt with k2
	11. Put the min-day-hour-sec follow by index-seq
	12. Encrypt min-day with k1 and hour-sec with k2
	13. If data meet the buf[97], simply skip it.

	Maximum data bytes: 120

 * */

	FILE *frd = fopen(RANDOM_FP, "r");
	if(frd == NULL)
	{
		CLOGE("failed1");
		return FAIL;
	}
	uint8_t bucket[256] = {0}; // To mark whether cur-buf is using
	uint32_t from;

	// Step 1
	const int chidx = _get_char_idx(frd);
	if(chidx == -1) goto ERR_OCR_RET4633;

	const int begidx = _get_begin_idx(frd);
	if(begidx == -1) goto ERR_OCR_RET4633;

	uint8_t buffer[256];
	if(_init_buffer(buffer, frd) != SUCC) goto ERR_OCR_RET4633;

#if DBG
	CLOGD("idx of characteristic: %d -> 0x%04x, idx of begin: %d", chidx, ch14s[chidx], begidx); 
	cl_print_bytes(buffer, 256);
#endif

	// Step 2
	buffer[BEG_IDX] = begidx;
	bucket[BEG_IDX] = 1;
#if DBG
{
	CLOGD("--> Put begin");
	int j, k;
	for(j = 0, k = 0; j < 256; j++)
	{
		if(j == BEG_IDX) printf("\e[41m");
		else if(j == (BEG_IDX + 1)) printf("\e[0m");

		printf("%02x", buffer[j]);
		if(k++ == 15)
		{
			k = 0;
			printf("\n");
		}
	}
	printf("\n");
}
#endif

	// Step 3
#if DBG
	int a151236, a151237;
#endif
	if(begidx == (BEG_IDX - 1))
	{
		buffer[begidx] = (uint8_t) ch14s[chidx];
		bucket[begidx] = 1;
		buffer[begidx + 2] = (uint8_t) (ch14s[chidx] >> 8);
		bucket[begidx + 2] = 1;
		from = begidx + 2;
#if DBG
		a151236 = begidx;
		a151237 = begidx + 2;
#endif
	}
	else if(begidx == 255)
	{
		buffer[begidx] = (uint8_t) ch14s[chidx];
		bucket[begidx] = 1;
		buffer[0] = (uint8_t) (ch14s[chidx] >> 8);
		bucket[0] = 1;
		from = 0;
#if DBG
		a151236 = begidx;
		a151237 = 0;
#endif
	}
	else
	{
		*((uint16_t *) (buffer + begidx)) = ch14s[chidx];
		bucket[begidx] = 1;
		bucket[begidx + 1] = 1;
		from = begidx + 1;
#if DBG
		a151236 = begidx;
		a151237 = begidx + 1;
#endif
	}
	const int ch1_idx = from; // For encrypt later
#if DBG
{
	CLOGD("--> Put the characteristic");
	int j, k;
	for(j = 0, k = 0; j < 256; j++)
	{
		if(j == a151236 || j == a151237) printf("\e[41m");
		else printf("\e[0m");

		printf("%02x", buffer[j]);
		if(k++ == 15)
		{
			k = 0;
			printf("\e[0m\n");
		}
	}
	printf("\e[0m\n");
}
#endif

	// Step 4, 5, 6
	GFROM();
	const uint8_t key1 = buffer[from];
	bucket[from] = 1;
#if DBG
	a151236 = from;
#endif
	GFROM();
	bucket[from] = 1;
	GFROM();
	buffer[from] = (uint8_t) plen;
	bucket[from] = 1;
	const int len_idx = from; // For encrypt later
	GFROM();
	bucket[from] = 1;
	GFROM();
	const uint8_t key2 = buffer[from];
	bucket[from] = 1;
#if DBG
	a151237 = from;
#endif
#if DBG
{
	CLOGD("--> The key1, key2 and len");
	CLOGD(" key1: 0x%02x, key2: 0x%02x, len: %d, [%d, %d]", key1, key2, plen, a151236, a151237);
	int j, k;
	for(j = 0, k = 0; j < 256; j++)
	{
		if(a151236 < a151237)
		{
			if(j >= a151236 && j <= a151237) printf("\e[41m");
			else printf("\e[0m");
		}
		else
		{
			if(j <= a151237 || j >= a151236) printf("\e[41m");
			else printf("\e[0m"); 
		}

		printf("%02x", buffer[j]);
		if(k++ == 15)
		{
			k = 0;
			printf("\e[0m\n");
		}
	}
	printf("\e[0m\n");
}
#endif

	// Step 7
	buffer[ch1_idx] ^= key2;
	buffer[len_idx] ^= key1;
#if DBG
{
	CLOGD("--> Encrypt the ch1 and len");
	int j, k;
	for(j = 0, k = 0; j < 256; j++)
	{
		if(j == ch1_idx || j == len_idx) printf("\e[41m");
		else printf("\e[0m");

		printf("%02x", buffer[j]);
		if(k++ == 15)
		{
			k = 0;
			printf("\e[0m\n");
		}
	}
	printf("\e[0m\n");
}
#endif

	// Step 8 ~ Step 13
	const int idx_amt = (int) ceil((double) plen / 4.0/*4 bytes data per index*/);
	int from2 = from;
#if DBG
	CLOGD("index amount: %d, from: %d", idx_amt, from2);
#endif
{
	int i;
	// make placeholder
	int arr1[64] = {0};
	for(i = 0; i < idx_amt; i++)
	{
		from2 = _get_free_pos(bucket, from2);
		if(from2 == -1)
		{
			CLOGE("failed3940");
			return FAIL;
		}
		bucket[from2] = 1;
		arr1[i] = from2;
	}

	// put the time
{
	time_t t1 = time(NULL);
	struct tm *t2 = localtime(&t1);
#if DBG
	CLOGD("day: %d, hour: %d, min: %d, sec: %d", t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec);
#endif
	const uint8_t tarr[] = {(uint8_t) t2->tm_min, (uint8_t) t2->tm_mday, (uint8_t) t2->tm_hour, (uint8_t) t2->tm_sec};
	for(i = 0; i < 4; i++)
	{
		from2 = _get_free_pos(bucket, from2);
		if(from2 == -1)
		{
			CLOGE("failed2822");
			return FAIL;
		}
		buffer[from2] = i < 2 ? tarr[i] ^ key1 : tarr[i] ^ key2;
#if DBG
		CLOGD("time cipher: %02x", buffer[from2]);
#endif
		bucket[from2] = 1;
	}
}

	// put the index and data
	for(i = 0; i < idx_amt; i++)
	{
		const int idx = _get_data_idx(bucket, frd);
		if(idx == -1)
		{
			CLOGE("failed1241");
			return FAIL;
		}
		from = arr1[i];
#if DBG
		CLOGD("idx: %d", idx);
#endif
		buffer[from] = ((from & 1) == 1) ? idx ^ key1 : idx ^ key2;
		_put_data(plain + (i << 2), plen - (i << 2), from, idx, key1, key2, bucket, buffer);
	}
}

	fclose(frd);

	// Output the cipher data
	memcpy(cipher, buffer, 256);
	*clen = 256;

	return SUCC;

ERR_OCR_RET4633:
	fclose(frd);
	return FAIL;
}

static int _get_next_idx(const int cur_idx)
{
	if(cur_idx == 255) return 0;
	if(cur_idx == (BEG_IDX - 1)) return BEG_IDX + 1;
	return cur_idx + 1;
}
#define NEXT() \
	idx = _get_next_idx(idx)

Ret cl_klciph_dec(uint8_t *cipher, int clen, uint8_t *plain, int *plen, int max_plen)
{
	TRACE();
	if(cipher == NULL || clen != 256 || plain == NULL || plen == NULL) return FAIL;

/*
	 1. Get the begin-index
	 2. Get the encrypted characteristic
	 3. Get the key1, key2 and len
	 4. Decrypt the characteristic and check it
	 5. Decrypt the len and check it
	 6. Calculate the amount of idx
	 7. Get and decrypt the time and check it
	 8. Get and decrypt the indices and get the data
 * */

	uint8_t buffer[clen];
	memcpy(buffer, cipher, clen);
	uint8_t plain_buf[MAX_PLAIN_BYTES];

	int idx;
	const int begidx = buffer[BEG_IDX];
#if DBG
	CLOGD("begin index: %d", begidx);
#endif
	idx = begidx;
	const uint8_t ch2 = buffer[idx];
	NEXT();
	uint8_t ch1 = buffer[idx];
	NEXT();
	const uint8_t key1 = buffer[idx];
	NEXT();
	NEXT();
	const uint8_t _len = buffer[idx];
	NEXT();
	NEXT();
	const uint8_t key2 = buffer[idx];
	ch1 ^= key2;
	const uint16_t ch14 = (uint16_t) (ch2 | ((ch1 << 8) & 0xff00));
	const uint8_t len = _len ^ key1;
#if DBG
	CLOGD("key1: 0x%02x, key2: 0x%02x, characteristic: 0x%04x, len: %d", key1, key2, ch14, len);
#endif
{
	int i;
	bool found = false;
	for(i = 0; i < CH14S_SZ; i++)
	{
		if(ch14 == ch14s[i])
		{
			found = true;
			break;
		}
	}
	if(!found)
	{
		CLOGE("failed2810");
		return FAIL;
	}
}

	if(len > MAX_PLAIN_BYTES)
	{
		CLOGE("failed1129");
		return FAIL;
	}

	const uint8_t idx_amt = (uint8_t) ceil((double) len / 4.0);
	const uint8_t bali = len % 4;
#if DBG
	CLOGD("amount of idx: %d, byte amount of last index: %d", idx_amt, bali);
#endif

	// Step 7
	const int _idx = idx;
{
	int i;
	for(i = 0; i < idx_amt; i++)
		NEXT();
}
	NEXT();
	const uint8_t min = buffer[idx] ^ key1;
	NEXT();
	const uint8_t day = buffer[idx] ^ key1;
	NEXT();
	const uint8_t hour = buffer[idx] ^ key2;
	NEXT();
	const uint8_t sec = buffer[idx] ^ key2;

	time_t t1 = time(NULL);
	struct tm *t2 = localtime(&t1);
	const uint8_t n_min = (uint8_t) t2->tm_min;
	const uint8_t n_day = (uint8_t) t2->tm_mday;
	const uint8_t n_hour = (uint8_t) t2->tm_hour;
	const uint8_t n_sec = (uint8_t) t2->tm_sec;
#if DBG
	CLOGD("The time: %d, %d:%d:%d", day, hour, min, sec);
	CLOGD("     now: %d, %d:%d:%d", n_day, n_hour, n_min, n_sec);
#endif
	if(day != n_day || hour != n_hour)
	{
		CLOGE("failed4524");
		return FAIL;
	}
	const int sec1 = min * 60 + sec;
	const int sec_diff = n_min * 60 + n_sec - sec1;
	if(sec_diff < 0 || sec_diff > 6)
	{
		CLOGE("failed948");
		return FAIL;
	}
	idx = _idx;

	// Step 8
	int j, k;
	for(j = 0, k = 0; j < idx_amt; j++)
	{
		NEXT();
		uint8_t where = (idx & 1) ? buffer[idx] ^ key1: buffer[idx] ^ key2;
#if DBG
		CLOGD("where: %d", where);
#endif
		plain_buf[k++] = (idx & 1) ? buffer[where] ^ key2 : buffer[where] ^ key1;
		if(where == 255) where = 0;
		else where++;

		plain_buf[k++] = (idx & 1) ? buffer[where] ^ key2 : buffer[where] ^ key1;
		if(where == 255) where = 0;
		else where++;

		plain_buf[k++] = (idx & 1) ? buffer[where] ^ key2 : buffer[where] ^ key1;
		if(where == 255) where = 0;
		else where++;

		plain_buf[k++] = (idx & 1) ? buffer[where] ^ key2 : buffer[where] ^ key1;
		if(where == 255) where = 0;
		else where++;
	}
	k -= ((4 - bali) % 4);
#if DBG
	CLOGD("cnt parsed: %d, len: %d", k, len);
#endif
	if(k != len)
	{
		CLOGE("failed737");
		return FAIL;
	}

	if(max_plen < k)
	{
		CLOGE("buf sz exceed");
		return FAIL;
	}

	memcpy(plain, plain_buf, k);
	*plen = k;

	return SUCC;
}

// Since 2025-11-15 05:40:58, klon at home.
