#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/rsa.h>

#include "cl_def.h"
#include "cl_ccll.h"
#include "cl_log.h"
#include "cl_alloc.h"
#include "cl_list.h"
#include "cl_event.h"
#include "cl_wait.h"
#include "cl_timer.h"
#include "cl_sh.h"
#include "cl_ini.h"
#include "cl_txt.h"
#include "cl_rsa.h"

TAG = "test";

#define DONE printf("\e[32m%s\e[0m done\n", __FUNCTION__)
#define LOG(fmt, args...) \
	printf("\e[36m"); \
	printf(fmt, ##args); \
	printf("\e[0m\n")
#define LTRACE() \
	LOG("__ %s", __FUNCTION__)

/******************************************
 **             common begin             **
 *****************************************/
static void test_alloc()
{
	LTRACE();
	char *buf1 = MALLOC(32);
	LOG("addr of buf1: %p", buf1);
	cl_iter_objs();
	FREE(buf1);
	cl_iter_objs();

	DONE;
}

static void test_txt()
{
	LTRACE();
	char *txt = " \n\
		this is line1\n\
		line2\r\n\
line3   \n\
\n\
\n\
   The last line";

	char line[64];
	char *_txt = txt;
	int off;
	int cnt = 0;
	while((off = cl_txt_pos_line(line, 64, _txt)) != -1)
	{
		LOG("--->%s<", line);
		if(off == 0) break;
		_txt += off;
		cnt++;
	}
	assert(cnt == 6);

	DONE;
}

static void test_common()
{
	LTRACE();
	test_alloc();
	test_txt();
}

/******************************************
 **              queue begin             **
 *****************************************/
static void test_list()
{
	typedef struct {
		int id;
		char name[16];
		int age;
		CLIST list;
	} Employee;

	CRE_LIST_HEAD(emps);
	assert(&emps != NULL);
	assert(emps.next == &emps);
	assert(emps.prev == &emps);

	int i;
#define S 100
	for(i = 0; i < S; i++)
	{
		Employee *emp = (Employee *) malloc(sizeof(Employee));
		assert(emp != NULL);
		emp->id = i;
		sprintf(emp->name, "employee%03d", i);
		emp->age = i << 2;
		init_list_node(&emp->list);

		list_add(&emp->list, &emps);
		assert(list_empty(&emps) == false);
	}
	// Check the size of list
	assert(list_size(&emps) == S);
	// Check the value
	CLIST *node = &emps;
	for(i = 0; i < S; i++)
	{
		CLIST *a = node->next;
		assert(a != NULL);
		Employee *b = container_of(a, Employee, list);
		assert(b != NULL);
		assert(b->id == i);
		assert(b->age == (i << 2));
		assert(strlen(b->name) == 11);
		assert(strncmp(b->name, "employee", 8) == 0);
		node = node->next;
	}
	// Check remove node from list
	node = &emps;
	for(i = 0; i < S; i++)
	{
		CLIST *a = node->next;
		assert(a != NULL);
		Employee *b = container_of(a, Employee, list);
		assert(b != NULL);
		list_del(a);
		assert(list_size(&emps) == (S - i - 1));
		free(b);
	}
#undef S

	DONE;
}

static void test_queue()
{
	test_list();
}

/******************************************
 **              event begin             **
 *****************************************/
typedef struct {
	char txt[18];
	int no;
} TST_EVT_DAT1;

typedef struct {
	TST_EVT_DAT1 *dat1;
	int no;
} TST_EVT_DAT2;

typedef struct {
	char *a;
	int no;
} TST_EVT_DAT3;

static void _tst_evt_dat1_free_fun(void *data)
{
	LTRACE();
	if(data == NULL) return;

	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	LOG("freeing1 no: %d", dat1->no);
	FREE(dat1);
}

static void _tst_evt_dat2_free_fun(void *data)
{
	LTRACE();
	if(data == NULL) return;

	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	_tst_evt_dat1_free_fun(dat2->dat1);
	LOG("freeing2 no: %d", dat2->no);
	FREE(dat2);
}

static void _tst_evt_dat3_free_fun(void *data)
{
	LTRACE();
	if(data == NULL) return;

	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	LOG("freeing3 no: %d", dat3->no);
	FREE(dat3);
}

static Bool _tst_evt_cb1(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	LOG("txt: %s, no: %d", dat1->txt, dat1->no);

	return false;
}

static Bool _tst_evt_cb2(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	TST_EVT_DAT1 *dat1 = dat2->dat1;
	LOG("txt: %s, no: %d, no2: %d", dat1->txt, dat1->no, dat2->no);

	return false;
}

static Bool _tst_evt_cb3(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	LOG("a: %s, no: %d", dat3->a, dat3->no);

	return false;
}

static Bool _tst_evt_cb20(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb21(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb22(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb23(uint16_t evt_no, void *data)
{
	LTRACE();
	_tst_evt_cb1(evt_no, data);
	return true;
}

static void test_event()
{
	LTRACE();

	// 1. Just publish
	LOG("\n\ncase 1");
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	Ret ret = cl_evt_pub(12, (void *) dat1, _tst_evt_dat1_free_fun);
	LOG("ret of pub evt-12: %d", ret);

	SLEEP(1);

	// 2. Just publish too
	LOG("\n\ncase 2");
	TST_EVT_DAT1 *dat1_2 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	ret = cl_evt_pub(13, (void *) dat1_2, _tst_evt_dat1_free_fun);
	LOG("ret of pub evt-13: %d", ret);

	SLEEP(1);

	// 3. Just publish too too
	LOG("\n\ncase 3");
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) MALLOC(sizeof(TST_EVT_DAT2));
	ret = cl_evt_pub(14, (void *) dat2, _tst_evt_dat2_free_fun);
	LOG("ret of pub evt-14: %d", ret);

	SLEEP(1);

	// 4. Sub and Pub
	LOG("\n\ncase 4");
	ret = cl_evt_sub(15, _tst_evt_cb1);
	LOG("ret of sub evt-15: %d", ret);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "Hello guy!");
	dat1->no = 185;
	ret = cl_evt_pub(15, (void *) dat1, _tst_evt_dat1_free_fun);
	LOG("ret of pub evt-15: %d", ret);
	SLEEP(1);
	cl_evt_unsub(15, _tst_evt_cb1);

	SLEEP(1);

	// 5. Single sub and twice publish
	LOG("\n\ncase 5");
	ret = cl_evt_sub(16, _tst_evt_cb1);
	LOG("ret of sub evt-16: %d", ret);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "Msg 15, body her");
	dat1->no = 995;
	ret = cl_evt_pub(15, (void *) dat1, _tst_evt_dat1_free_fun);
	LOG("ret of pub evt-15: %d", ret);
	dat1_2 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1_2->txt, "Msg 16, body her");
	dat1_2->no = 996;
	ret = cl_evt_pub(16, (void *) dat1_2, _tst_evt_dat1_free_fun);
	LOG("ret of pub evt-16: %d", ret);
	SLEEP(1);
	cl_evt_unsub(16, _tst_evt_cb1);
	dat1_2->txt[3] = 22;
	dat1_2->no = 998; // It should cause care-dump

	SLEEP(1);

	// 6. Multiply sub and multiple pub
	LOG("\n\ncase 6");
	assert(cl_evt_sub(17, _tst_evt_cb1) == 0);
	assert(cl_evt_sub(18, _tst_evt_cb2) == 0);
	assert(cl_evt_sub(19, _tst_evt_cb3) == 0);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "msg 17-dat1");
	dat1->no = 171;
	dat2 = (TST_EVT_DAT2 *) MALLOC(sizeof(TST_EVT_DAT2));
	dat2->dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat2->dat1->txt, "msg-18-dat2-dat1");
	dat2->dat1->no = 1821;
	dat2->no = 182;
	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) MALLOC(sizeof(TST_EVT_DAT3));
	dat3->a = "msg-19,dat3";
	dat3->no = 19;
	assert(cl_evt_pub(19, (void *) dat3, _tst_evt_dat3_free_fun) == 0);
	assert(cl_evt_pub(17, (void *) dat1, _tst_evt_dat1_free_fun) == 0);
	assert(cl_evt_pub(18, (void *) dat2, _tst_evt_dat2_free_fun) == 0);
	SLEEP(1);
	assert(cl_evt_unsub(17, _tst_evt_cb1) == SUCC);
	assert(cl_evt_unsub(18, _tst_evt_cb2) == SUCC);
	assert(cl_evt_unsub(19, _tst_evt_cb3) == SUCC);
	assert(cl_evt_unsub(18, _tst_evt_cb3) == FAIL);

	SLEEP(1);

	// 7. Multiple listener
	LOG("\n\ncase 7");
	assert(cl_evt_sub(20, _tst_evt_cb20) == 0);
	assert(cl_evt_sub(20, _tst_evt_cb21) == 0);
	assert(cl_evt_sub(20, _tst_evt_cb22) == 0);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "evt-20,,,");
	dat1->no = 20;
	assert(cl_evt_pub(20, (void *) dat1, _tst_evt_dat1_free_fun) == SUCC);
	SLEEP(1);
	assert(cl_evt_unsub(20, _tst_evt_cb20) == SUCC);
	assert(cl_evt_unsub(20, _tst_evt_cb21) == SUCC);
	assert(cl_evt_unsub(20, _tst_evt_cb22) == SUCC);

	SLEEP(1);

	// 8. Intercept event
	LOG("\n\ncase 8");
	assert(cl_evt_sub(21, _tst_evt_cb23) == 0);
	assert(cl_evt_sub(21, _tst_evt_cb21) == 0);
	assert(cl_evt_sub(21, _tst_evt_cb22) == 0);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "evt-21-===");
	dat1->no = 21;
	assert(cl_evt_pub(21, (void *) dat1, _tst_evt_dat1_free_fun) == SUCC);
	SLEEP(1);
	assert(cl_evt_unsub(21, _tst_evt_cb23) == SUCC);
	assert(cl_evt_unsub(21, _tst_evt_cb21) == SUCC);
	assert(cl_evt_unsub(21, _tst_evt_cb22) == SUCC);

	SLEEP(1);

	SLEEP(3);

	cl_iter_objs();
	assert(cl_allocing_cnt() == 0);
	DONE;
}

/******************************************
 **              timer begin             **
 ******************************************/
static int g_tmr_sym;
static void _tmr_cb1()
{
	LOG("timer cb1");
	CLOGI("-----> tick 1-");
	g_tmr_sym = 1;
}

static void _tmr_cb2()
{
	LOG("timer cb2");
	g_tmr_sym++;
}

static void _tmr_cb3()
{
	g_tmr_sym++;
}

static void test_timer()
{
	LTRACE();
	LOG("1. Set a 50ms timer in once");
	CLOGI("-----> tick 1");
	g_tmr_sym = 0;
	assert(cl_timer_set(0, 50, 1, _tmr_cb1) == SUCC);
	SLEEP_MS(55);
	assert(g_tmr_sym == 1);
	assert(cl_timer_count() == 0);

	LOG("2. Set a 50ms timer in 10 times");
	g_tmr_sym = 0;
	assert(cl_timer_set(0, 50, 10, _tmr_cb2) == SUCC);
	{
		int i;
		for(i = 0; i < 10; i++)
		{
			SLEEP_MS(51);
			assert(g_tmr_sym == (i + 1));
		}
	}
	assert(cl_timer_count() == 0);

	LOG("3. Set a 50ms timer in loop forever");
#if 1
	g_tmr_sym = 0;
	assert(cl_timer_set(0, 50, 0, _tmr_cb3) == SUCC);
	{
		int i;
		int pre;
		for(i = 0; i < 2000/*100s*/; i++)
		{
			pre = g_tmr_sym;
			SLEEP_MS(55);
			assert(g_tmr_sym > pre);
		}
		LOG("case 3 done");
	}
	assert(cl_timer_cancel(_tmr_cb3) == SUCC);
	{
		int i, j;
		for(i = 0; i < 20; i++)
		{
			j = g_tmr_sym;
			SLEEP_MS(55);
			assert(g_tmr_sym == j);
		}
	}
	assert(cl_timer_count() == 0);
#else
	LOG("  skipped");
#endif

	LOG("4. Set a 10s timer with one time repeat");
#if 1
	g_tmr_sym = 0;
	assert(cl_timer_set(10, 0, 2, _tmr_cb2) == SUCC);
	SLEEP(11);
	assert(g_tmr_sym == 1);
	SLEEP(11);
	assert(g_tmr_sym == 2);
	SLEEP(1);
	assert(cl_timer_count() == 0);
#else
	LOG("  skipped");
#endif

	LOG("5. Set a 10s and 200ms timer with no repeat");
#if 1
	g_tmr_sym = 0;
	assert(cl_timer_set(10, 200, 1, _tmr_cb2) == SUCC);
	SLEEP(11);
	assert(g_tmr_sym == 1);
	SLEEP(1);
	assert(cl_timer_count() == 0);
#else
	LOG("  skipped");
#endif

	LOG("6. Set two timer in 1s and 3s with no repeat");
	g_tmr_sym = 0;
	assert(cl_timer_set(1, 0, 1, _tmr_cb2) == SUCC);
	assert(cl_timer_set(3, 0, 1, _tmr_cb3) == SUCC);
	SLEEP(1);
	SLEEP_MS(200);
	assert(g_tmr_sym == 1);
	SLEEP(2);
	assert(g_tmr_sym == 2);
	SLEEP_MS(100);
	assert(cl_timer_count() == 0);

	LOG("7. Set two timer in both 1s with no repeat");
	g_tmr_sym = 0;
	assert(cl_timer_set(1, 0, 1, _tmr_cb2) == SUCC);
	assert(cl_timer_set(1, 0, 1, _tmr_cb3) == SUCC);
	SLEEP(1);
	SLEEP_MS(20);
	assert(g_tmr_sym == 2);
	SLEEP_MS(50);
	assert(cl_timer_count() == 0);

	SLEEP(3);

	DONE;
}

/******************************************
 **              testing sh              **
 ******************************************/
static void test_sh()
{
	LTRACE();

	int rlen;

	{
		LOG("1. No returning cmd");
#define FN1 "77564311009888"
		remove(FN1);
		assert(cl_sh_exec("touch " FN1, NULL, 0, NULL) == SUCC);
		struct stat st;
		assert(stat(FN1, &st) == 0);
		assert((st.st_mode & S_IFMT) == S_IFREG);
		assert(remove(FN1) == 0);
#undef FN1
	}

	{
		LOG("2. ");
		char result[4096] = {0};
		assert(cl_sh_exec("ls", result, 4096, &rlen) == SUCC);
		LOG("rlen: %d", rlen);
		LOG("result:\n%s\n", result);
		assert(rlen == strlen(result));
		assert(strlen(result) < 4096);
	}

	{
		LOG("3. ");
		char result[4] = {0};
		assert(cl_sh_exec("ls", result, 4, &rlen) == SUCC);
		LOG("rlen: %d", rlen);
		LOG("result:\n%s\n", result);
		assert(rlen == 3);
		assert(strlen(result) == 3);
	}

	LOG("4. invalid cmd");
	assert(cl_sh_exec("invalid-cmd", NULL, 0, NULL) == FAIL);

	LOG("5. ");
	char result[8] = {0};
	assert(cl_sh_exec("ls", result, 0, NULL) == SUCC);
	LOG("result:\n%s\n", result);
	assert(strlen(result) == 0);
	memset(result, 0, 8);
	assert(cl_sh_exec("ls", result, 8, NULL) == SUCC);
	LOG("result:\n%s\n", result);
	assert(strlen(result) == 7);
	assert(cl_sh_exec("ls", NULL, 8, NULL) == SUCC);
	assert(cl_sh_exec("ls", NULL, 0, NULL) == SUCC);

	DONE;
}


/******************************************
 **             testing cfg              **
 ******************************************/
static void test_cfg()
{
	LTRACE();
	const char *sec = "sec";
	const char *key = "key";
	char value[128];
	LOG("1. invalid fn");
	assert(cl_ini_get("ewqrq", NULL, key, value) == FAIL);
	assert(cl_ini_get("8888", sec, key, value) == FAIL);

	LOG("2. valid fn");
	const char *fn1 = "test/test1.ini";
	const char *fn2 = "test/test2.ini";
	const char *sec1 = "sec1";
	const char *sec2 = "sec2";
	const char *sec3 = "sec3";
	const char *sec4 = "sec4";
	const char *key1 = "key1";
	const char *key2 = "key.2";
	const char *key3 = "key4";
	const char *key4 = "key5";
	const char *key5 = "key6";
	const char *key6 = "key7";
	const char *key7 = "key11";
	const char *key8 = "key8";
	const char *value1 = "value1";
	const char *value2 = "value.2";
	const char *value3 = "value3";
	const char *value4 = "value4";
	const char *value5 = "value5";
	const char *value6 = "value6";
	const char *value7 = "val7";
	const char *value8 = "= value8";
	assert(cl_ini_get(fn1, NULL, key, value) == FAIL);
	assert(cl_ini_get(fn1, sec1, key1, value) == SUCC);
	assert(strcmp(value, value1) == 0);
	assert(cl_ini_get(fn1, sec1, key2, value) == SUCC);
	assert(strcmp(value, value2) == 0);
	assert(cl_ini_get(fn1, sec1, key3, value) == FAIL);
	assert(cl_ini_get(fn1, sec2, key1, value) == SUCC);
	assert(strcmp(value, value3) == 0);
	assert(cl_ini_get(fn1, sec2, key2, value) == FAIL);
	assert(cl_ini_get(fn1, sec3, key2, value) == FAIL);
	assert(cl_ini_get(fn1, sec3, key1, value) == SUCC);
	assert(strcmp(value, value4) == 0);
	assert(cl_ini_get(fn1, sec4, key2, value) == FAIL);
	assert(cl_ini_get(fn1, sec4, key3, value) == SUCC);
	assert(strcmp(value, value5) == 0);
	assert(cl_ini_get(fn1, sec4, key4, value) == SUCC);
	assert(strcmp(value, value6) == 0);
	assert(cl_ini_get(fn1, sec4, key5, value) == SUCC);
	assert(strcmp(value, value6) == 0);
	assert(cl_ini_get(fn1, sec4, key6, value) == SUCC);
	assert(strcmp(value, value7) == 0);
	assert(cl_ini_get(fn2, sec1, key1, value) == FAIL);
	assert(strcmp(value, value7) == 0);
	assert(cl_ini_get(fn2, sec4, key3, value) == FAIL);
	assert(cl_ini_get(fn2, NULL, key1, value) == SUCC);
	assert(strcmp(value, value1) == 0);
	assert(cl_ini_get(fn2, NULL, key2, value) == SUCC);
	assert(strcmp(value, value2) == 0);
	assert(cl_ini_get(fn2, NULL, key7, value) == SUCC);
	assert(strcmp(value, value4) == 0);
	assert(cl_ini_get(fn2, NULL, key3, value) == SUCC);
	assert(strcmp(value, value5) == 0);
	assert(cl_ini_get(fn2, NULL, key4, value) == SUCC);
	assert(strcmp(value, value6) == 0);
	assert(cl_ini_get(fn2, NULL, key5, value) == SUCC);
	assert(strcmp(value, value6) == 0);
	assert(cl_ini_get(fn2, NULL, key6, value) == SUCC);
	assert(strcmp(value, value7) == 0);
	assert(cl_ini_get(fn2, NULL, key8, value) == SUCC);
	assert(strcmp(value, value8) == 0);

	LOG("3. ini in memory");
	const char *keys[] = {
		"key1",
		"key2",
		"key3",
		"key4",
		"key5",
		"key6",
	};
	const char *vals[] = {
		"value1",
		"value2",
		"value3",
		"value4",
		"value5",
		"value6",
	};
	char *ini1 = "key1     =value1\n\
key2=value2\r\n\
key3 = value3\r\n\
key4      =value4\n\
		key5=     								value5\r\n\
		key6 = value6";
	{
		int i;
		char value[64];
		const size_t cnt = sizeof(keys) / sizeof(char *);
		for(i = 0; i < cnt; i++)
		{
			assert(cl_ini_get2(ini1, NULL, keys[i], value) == SUCC);
			LOG("key: >%s<, value: >%s<", keys[i], value);
			assert(strcmp(value, vals[i]) == 0);
		}
	}

	DONE;
}


/******************************************
 **             testing cipher             **
 ******************************************/
static void test_rsa()
{
	LTRACE();
	RSA *rsa;

	LOG("1. generate the RSA");
	rsa = NULL;
	assert(cl_rsa_gen(123, 2048, &rsa) == FAIL);
	assert(rsa == NULL);
	assert(cl_rsa_gen(123, 4096, &rsa) == FAIL);
	assert(rsa == NULL);
	assert(cl_rsa_gen(123, 456, &rsa) == FAIL);
	assert(rsa == NULL);
	assert(cl_rsa_gen(65537, 456, &rsa) == FAIL);
	assert(rsa == NULL);
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	cl_rsa_destroy(rsa);
	rsa = NULL;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	cl_rsa_destroy(rsa);

	LOG("2. export RSA to file");
	const char *pbk_fn1 = "out/pbk1.pem";
	const char *pbk_fn2 = "out/pbk2.pem";
	const char *pbk_fn3 = "out/pbk3.pem";
	const char *pbk_fn4 = "out/pbk4.pem";
	const char *pvk_fn1 = "out/pvk1.pem";
	const char *pvk_fn2 = "out/pvk2.pem";
	const char *pvk_fn3 = "out/pvk3.pem";
	const char *pvk_fn4 = "out/pvk4.pem";
	rsa = NULL;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_to_file(rsa, pbk_fn1, pvk_fn1) == SUCC);
	assert(cl_rsa_to_file(rsa, pbk_fn2, NULL) == SUCC);
	assert(cl_rsa_to_file(rsa, NULL, pvk_fn2) == SUCC);
	assert(cl_rsa_to_file(rsa, NULL, NULL) == SUCC);
	cl_rsa_destroy(rsa);
	rsa = NULL;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_to_file(rsa, pbk_fn3, pvk_fn3) == SUCC);
	assert(cl_rsa_to_file(rsa, pbk_fn4, NULL) == SUCC);
	assert(cl_rsa_to_file(rsa, NULL, pvk_fn4) == SUCC);
	assert(cl_rsa_to_file(rsa, NULL, NULL) == SUCC);
	cl_rsa_destroy(rsa);
	LOG("  You must check the export file manually");

	LOG("3. export RSA to memory");
	uint8_t pbk_bf1[512];
	uint8_t pvk_bf1[2048];
	size_t pbk_len1;
	size_t pvk_len1;
	rsa = NULL;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	memset(pbk_bf1, 0, 512);
	memset(pvk_bf1, 0, 2048);
	pbk_len1 = 0;
	pvk_len1 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, pvk_bf1, &pvk_len1) == SUCC);
	LOG("pbk_len1: %ld, pbk_bf1:\n%s\npvk_len1: %ld, pvk_bf1:\n%s\n", pbk_len1, pbk_bf1, pvk_len1, pvk_bf1);
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, NULL, pvk_bf1, &pvk_len1) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, pvk_bf1, NULL) == FAIL);
	memset(pbk_bf1, 0, 512);
	memset(pvk_bf1, 0, 2048);
	pbk_len1 = 0;
	pvk_len1 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, NULL, NULL) == SUCC);
	LOG("pbk_len1: %ld, pbk_bf1:\n%s\npvk_len1: %ld, pvk_bf1:\n%s\n", pbk_len1, pbk_bf1, pvk_len1, pvk_bf1);
	cl_rsa_destroy(rsa);
	uint8_t pbk_bf2[1024];
	uint8_t pvk_bf2[4096];
	size_t pbk_len2;
	size_t pvk_len2;
	rsa = NULL;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, pvk_bf2, &pvk_len2) == SUCC);
	LOG("[1] pbk_len2: %ld, pbk_bf2:\n%s\npvk_len2: %ld, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, NULL, pvk_bf2, &pvk_len2) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, pvk_bf2, NULL) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, NULL, pvk_bf2, NULL) == FAIL);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, NULL, NULL) == SUCC);
	LOG("[2] pbk_len2: %ld, pbk_bf2:\n%s\npvk_len2: %ld, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, NULL, &pvk_len2) == SUCC);
	LOG("[3] pbk_len2: %ld, pbk_bf2:\n%s\npvk_len2: %ld, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
	cl_rsa_destroy(rsa);

	LOG("4. encrypt and decrypt");
	char *words = "This is my testing data for RSA ciphering...";
	const size_t wd_len = strlen(words);
	LOG("words len: %ld", wd_len);
	uint8_t cipher_buf[600];
	uint8_t plain_buf[100];
	int plen, clen;
	memset(cipher_buf, 0, 600);
	memset(plain_buf, 0, 100);
	plen = -1;
	clen = -1;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_enc(rsa, true, words, wd_len, cipher_buf, &clen) == SUCC);
	assert(clen == 256);
	LOG("clen: %d", clen);
	assert(cl_rsa_dec(rsa, false, cipher_buf, clen, plain_buf, &plen) == SUCC);
	assert(plen == wd_len);
	LOG("plen: %d", plen);
	assert(strcmp(plain_buf, words) == 0);
	LOG("[1] text decrypted: %s", plain_buf);
	cl_rsa_destroy(rsa);
	memset(cipher_buf, 0, 600);
	memset(plain_buf, 0, 100);
	plen = -1;
	clen = -1;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_enc(rsa, true, words, wd_len, cipher_buf, &clen) == SUCC);
	assert(clen == 512);
	LOG("clen: %d", clen);
	assert(cl_rsa_dec(rsa, false, cipher_buf, clen, plain_buf, &plen) == SUCC);
	assert(plen == wd_len);
	LOG("plen: %d", plen);
	assert(strcmp(plain_buf, words) == 0);
	LOG("[2] text decrypted: %s", plain_buf);
	cl_rsa_destroy(rsa);
	memset(cipher_buf, 0, 600);
	memset(plain_buf, 0, 100);
	plen = -1;
	clen = -1;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_enc(rsa, false, words, wd_len, cipher_buf, &clen) == SUCC);
	assert(clen == 256);
	LOG("clen: %d", clen);
	assert(cl_rsa_dec(rsa, true, cipher_buf, clen, plain_buf, &plen) == SUCC);
	assert(plen == wd_len);
	LOG("plen: %d", plen);
	assert(strcmp(plain_buf, words) == 0);
	LOG("[3] text decrypted: %s", plain_buf);
	cl_rsa_destroy(rsa);
	memset(cipher_buf, 0, 600);
	memset(plain_buf, 0, 100);
	plen = -1;
	clen = -1;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_enc(rsa, false, words, wd_len, cipher_buf, &clen) == SUCC);
	assert(clen == 512);
	LOG("clen: %d", clen);
	assert(cl_rsa_dec(rsa, true, cipher_buf, clen, plain_buf, &plen) == SUCC);
	assert(plen == wd_len);
	LOG("plen: %d", plen);
	assert(strcmp(plain_buf, words) == 0);
	LOG("[4] text decrypted: %s", plain_buf);
	cl_rsa_destroy(rsa);
	memset(cipher_buf, 0, 600);
	memset(plain_buf, 0, 100);
	plen = -1;
	clen = -1;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	assert(cl_rsa_enc(rsa, true, words, wd_len, cipher_buf, &clen) == SUCC);
	assert(clen == 256);
	LOG("clen: %d", clen);
	assert(cl_rsa_dec(rsa, true, cipher_buf, clen, plain_buf, &plen) == FAIL);
	cl_rsa_destroy(rsa);

	DONE;
}

static void test_cipher()
{
	LTRACE();
	test_rsa();

	DONE;
}


/******************************************
 **             testing menu             **
 ******************************************/
static void test()
{
	//test_common();
	//test_queue();
	//test_event();
	//test_timer();
	//test_sh();
	//test_cfg();
	//test_cipher();
}

int main()
{
	if(cl_init(NULL) != SUCC)
	{
		LOG("ccll init failed");
		return -1;
	}
	LOG("libccll init succ");

	test();

	cl_deinit();

	return 0;
}
