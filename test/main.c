#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <openssl/rsa.h>

#include "cl_def.h"
#include "cl_ccll.h"
#include "cl_log_type.h"
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
#include "cl_klciph.h"
#include "cl_bytes.h"
#include "cl_thrdpool.h"
#include "cl_times.h"
#include "cl_mem.h"

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

static void test_times()
{
	LTRACE();
	time_t tt;
	const char *t1 = "2025-11-25 22:29:00";
	const char *t2 = "2025-11-25 22:29:59";
	const char *t3 = "2025-11-25 22:00:00";
	const char *t4 = "2025-11-25 22:59:00";
	const char *t5 = "2025-11-25 00:29:00";
	const char *t6 = "2025-11-25 23:29:00";
	const char *t7 = "2025-12-01 23:29:00";
	const char *t8 = "2025-11-30 23:29:00";
	const char *t9 = "2025-01-25 23:29:00";
	const char *t10 = "2025-12-25 23:29:00";
	const char *t11 = "1970-11-25 23:29:00";
	const char *t12 = "2099-11-25 23:29:00";
	const char *t13 = "1970-01-01 00:00:00";
	const char *t14 = "1969-01-01 00:00:00";

	LOG("1. time str2int1");
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t1, &tt) == SUCC);
	LOG(" [ 1]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t2, &tt) == SUCC);
	LOG(" [ 2]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t3, &tt) == SUCC);
	LOG(" [ 3]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t4, &tt) == SUCC);
	LOG(" [ 4]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t5, &tt) == SUCC);
	LOG(" [ 5]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t6, &tt) == SUCC);
	LOG(" [ 6]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t7, &tt) == SUCC);
	LOG(" [ 7]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t8, &tt) == SUCC);
	LOG(" [ 8]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t9, &tt) == SUCC);
	LOG(" [ 9]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t10, &tt) == SUCC);
	LOG(" [10]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t11, &tt) == SUCC);
	LOG(" [11]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t12, &tt) == SUCC);
	LOG(" [12]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t13, &tt) == SUCC);
	LOG(" [13]time_t: %ld", tt);
	tt = (time_t) -1;
	assert(cl_time_date2sec1(t14, &tt) == FAIL);
	LOG(" [14]time_t: %ld", tt);

	DONE;
}

static void test_common()
{
	LTRACE();
	test_alloc();
	test_txt();
	test_times();
}

/******************************************
 **              queue begin             **
 *****************************************/
#if 0
static void _emp_free_fun(CLIST *node)
{
	typedef struct {
		int id;
		char name[16];
		int age;
		CLIST list;
	} Employee;

	Employee *emp = container_of(node, Employee, list);
	LOG("freeing %d", emp->id);
	FREE(emp);
}
#endif

static void test_list()
{
	typedef struct {
		int id;
		char name[16];
		int age;
		CLIST list;
	} Employee;

	LOG("1. ");
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

	LOG("2. remove all elements [1]");
	const uint32_t cnt1 = cl_allocing_cnt();
	LOG("cnt1: %d", cnt1);
	CRE_LIST_HEAD(emps2);
	assert(&emps2 != NULL);
	assert(emps2.next == &emps2);
	assert(emps2.prev == &emps2);
	for(i = 0; i < 20; i++)
	{
		Employee *emp = (Employee *) MALLOC(sizeof(Employee));
		assert(emp != NULL);
		emp->id = i;
		list_add(&emp->list, &emps2);
		assert(list_size(&emps2) == (i + 1));
	}
	assert(list_size(&emps2) == i);

	const uint32_t cnt2 = cl_allocing_cnt();
	LOG("cnt2: %d", cnt2);
	CLIST *li_emp1;
	i = 0;
	list_for_each(li_emp1, &emps2)
	{
		CLIST *pop;
		list_pop(li_emp1, pop);
		Employee *emp3 = container_of(pop, Employee, list);
		assert(emp3->id == i++);
		FREE(emp3);
	}
	assert(list_size(&emps2) == 0);
	const int cnt3 = cl_allocing_cnt();
	LOG("cnt3: %d/%d", cnt1, cnt3);
	assert(cnt1 == cnt3);

	LOG("3. remove all elements [2]");
	const uint32_t cnt4 = cl_allocing_cnt();
	LOG("cnt4: %d", cnt4);
	CRE_LIST_HEAD(emps3);
	assert(&emps3 != NULL);
	assert(emps3.next == &emps3);
	assert(emps3.prev == &emps3);
	for(i = 0; i < 47; i++)
	{
		Employee *emp = (Employee *) MALLOC(sizeof(Employee));
		assert(emp != NULL);
		emp->id = i;
		list_add(&emp->list, &emps3);
		assert(list_size(&emps3) == (i + 1));
	}
	assert(list_size(&emps3) == i);

	const uint32_t cnt5 = cl_allocing_cnt();
	LOG("cnt5: %d", cnt5);
	Employee *emp3;
	i = 0;
	list_for_each2(emp3, &emps3, list)
	{
		Employee *pop;
		list_pop2(emp3, pop, Employee, list);
		assert(pop->id == i++);
		FREE(pop);
	}
	assert(list_size(&emps3) == 0);
	const int cnt6 = cl_allocing_cnt();
	LOG("cnt6: %d/%d", cnt4, cnt6);
	assert(cnt4 == cnt6);

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

static bool _tst_evt_cb1(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	LOG("txt: %s, no: %d", dat1->txt, dat1->no);

	return false;
}

static bool _tst_evt_cb2(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	TST_EVT_DAT1 *dat1 = dat2->dat1;
	LOG("txt: %s, no: %d, no2: %d", dat1->txt, dat1->no, dat2->no);

	return false;
}

static bool _tst_evt_cb3(uint16_t evt_no, void *data)
{
	LTRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	LOG("a: %s, no: %d", dat3->a, dat3->no);

	return false;
}

static bool _tst_evt_cb20(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static bool _tst_evt_cb21(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static bool _tst_evt_cb22(uint16_t evt_no, void *data)
{
	LTRACE();
	return _tst_evt_cb1(evt_no, data);
}

static bool _tst_evt_cb23(uint16_t evt_no, void *data)
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
	int pbk_len1;
	int pvk_len1;
	rsa = NULL;
	assert(cl_rsa_gen(65537, 2048, &rsa) == SUCC);
	assert(rsa != NULL);
	memset(pbk_bf1, 0, 512);
	memset(pvk_bf1, 0, 2048);
	pbk_len1 = 0;
	pvk_len1 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, pvk_bf1, &pvk_len1) == SUCC);
	LOG("pbk_len1: %d, pbk_bf1:\n%s\npvk_len1: %d, pvk_bf1:\n%s\n", pbk_len1, pbk_bf1, pvk_len1, pvk_bf1);
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, NULL, pvk_bf1, &pvk_len1) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, pvk_bf1, NULL) == FAIL);
	memset(pbk_bf1, 0, 512);
	memset(pvk_bf1, 0, 2048);
	pbk_len1 = 0;
	pvk_len1 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf1, &pbk_len1, NULL, NULL) == SUCC);
	LOG("pbk_len1: %d, pbk_bf1:\n%s\npvk_len1: %d, pvk_bf1:\n%s\n", pbk_len1, pbk_bf1, pvk_len1, pvk_bf1);
	cl_rsa_destroy(rsa);
	uint8_t pbk_bf2[1024];
	uint8_t pvk_bf2[4096];
	int pbk_len2;
	int pvk_len2;
	rsa = NULL;
	assert(cl_rsa_gen(65537, 4096, &rsa) == SUCC);
	assert(rsa != NULL);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, pvk_bf2, &pvk_len2) == SUCC);
	LOG("[1] pbk_len2: %d, pbk_bf2:\n%s\npvk_len2: %d, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, NULL, pvk_bf2, &pvk_len2) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, pvk_bf2, NULL) == FAIL);
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, NULL, pvk_bf2, NULL) == FAIL);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, NULL, NULL) == SUCC);
	LOG("[2] pbk_len2: %d, pbk_bf2:\n%s\npvk_len2: %d, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
	memset(pbk_bf2, 0, 1024);
	memset(pvk_bf2, 0, 4096);
	pbk_len2 = 0;
	pvk_len2 = 0;
	assert(cl_rsa_to_bytes(rsa, pbk_bf2, &pbk_len2, NULL, &pvk_len2) == SUCC);
	LOG("[3] pbk_len2: %d, pbk_bf2:\n%s\npvk_len2: %d, pvk_bf2:\n%s\n", pbk_len2, pbk_bf2, pvk_len2, pvk_bf2);
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

static void test_klciph()
{
	LTRACE();
	char *plain = "1234567890";
	uint8_t cipher[256];
	int clen;

	uint8_t fullbuf[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

	LOG("1. ");
	assert(cl_klciph_enc(NULL, 1, cipher, &clen) == FAIL);
	assert(cl_klciph_enc(plain, 0, cipher, &clen) == FAIL);
	assert(cl_klciph_enc(plain, 1, cipher, &clen) == SUCC);
	assert(cl_klciph_enc(plain, 120, cipher, &clen) == SUCC);
	assert(cl_klciph_enc(plain, 121, cipher, &clen) == FAIL);
	assert(cl_klciph_enc(plain, strlen(plain), NULL, &clen) == FAIL);
	assert(cl_klciph_enc(plain, strlen(plain), cipher, NULL) == FAIL);
	assert(cl_klciph_enc(plain, strlen(plain), cipher, &clen) == SUCC);
	uint8_t plain2[256];
	int plen2;
	assert(cl_klciph_dec(NULL, clen, plain2, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, 0, plain2, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, 1, plain2, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, 128, plain2, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, 255, plain2, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, 256, plain2, &plen2, 256) == SUCC);
	assert(cl_klciph_dec(cipher, clen, NULL, &plen2, 256) == FAIL);
	assert(cl_klciph_dec(cipher, clen, plain2, NULL, 256) == FAIL);
	assert(cl_klciph_dec(cipher, clen, plain2, &plen2, 256) == SUCC);
	plain2[plen2] = 0;
	LOG(" plain2: %s, len: %ld", plain2, strlen(plain2));
	assert(strlen(plain2) == strlen(plain));
	assert(strcmp(plain2, plain) == 0);

	LOG("2.");
	FILE *frd = fopen("/dev/random", "r");
	int cnt = 1000;
	while(cnt-- > 0)
	{
#if 1
		uint8_t begin = 255, len = 255;
		while(begin == 255) fread(&begin, 1, 1, frd);
		while((len > 120) || (len < 1) || ((256 - begin) < len)) fread(&len, 1, 1, frd);
#else
		uint8_t begin = 5, len = 21;
#endif
		LOG("begin: %d, len: %d", begin, len);

		assert(cl_klciph_enc(fullbuf + begin, len, cipher, &clen) == SUCC);
		cl_print_bytes(cipher, 256);

		SLEEP_MS(10);

		LOG("--------- decrypt");
		assert(cl_klciph_dec(cipher, clen, plain2, &plen2, 256) == SUCC);
		assert(plen2 == len);
		assert(memcmp(plain2, fullbuf + begin, len) == 0);
		LOG("--- done");
	}
	fclose(frd);

	DONE;
}

static void test_cipher()
{
	LTRACE();
	test_rsa();
	test_klciph();

	DONE;
}


/******************************************
 **            thrdpool begin            **
 ******************************************/
static void _trpo_wk_fun(CLTrPoArg *args)
{
	CLTrPoArg *arg = (CLTrPoArg *) args;
	const int id = arg->thrd_id;
	const int num = *((int *) arg->args);
	LOG("hi~ %d at %d", num, id);
	SLEEP_MS(765);
	LOG("bye! %d", num);
}

static void test_thrdpol()
{
	const size_t alcnt1 = cl_allocing_cnt();
	const int max_amt = get_nprocs_conf() << 2/*refer to thrdpool.MAX_THRD_CNT_SHIFT_BIT*/;
	LOG("max sub-thrd amount: %d", max_amt);
	const char *name = "nametrpo";
	int id;

	LOG("1. Invalid param for create");
	assert(cl_trpo_create(0, NULL) == -1);
	assert(cl_trpo_create(max_amt + 1, NULL) == -1);
	assert(cl_trpo_create(0, name) == -1);
	assert(cl_trpo_create(max_amt + 1, name) == -1);


	LOG("2. Basical create thread-pool");
	id = cl_trpo_create(10, NULL);
	LOG(" id of thrd-pol: %d", id);
	assert(id != -1);
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());
	LOG(" creating max-amt pol-thrd");
	id = cl_trpo_create(max_amt, NULL);
	assert(id != -1);
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());


	LOG("3. Sub-thread limits test");
	LOG(" creating max-amt pol-thrd");
	id = cl_trpo_create(max_amt, name);
	assert(id != -1);
	LOG(" creating one more pol-thrd");
	assert(cl_trpo_create(1, name) == -1);
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());


	LOG("4. Repeat creating");
	LOG(" creating max-amt pol-thrd");
	id = cl_trpo_create(max_amt, name);
	assert(id != -1);
	LOG(" creating one more pol-thrd");
	assert(cl_trpo_create(1, name) == -1);
	LOG(" destroy the allocated pol-thrd");
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());
	LOG(" creating max-amt pol-thrd again");
	id = cl_trpo_create(max_amt, name);
	assert(id != -1);
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());


	LOG("5. Post work thread");
	id = cl_trpo_create(4, NULL);
	assert(id != -1);
	int arg1 = 1, arg2 = 2, arg3 = 3, arg4 = 4, arg5 = 5, arg6 = 6;
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg1) == SUCC);
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg2) == SUCC);
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg3) == SUCC);
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg4) == SUCC);
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg5) == SUCC);
	assert(cl_trpo_post(id, _trpo_wk_fun, &arg6) == SUCC);
	SLEEP(4);
	LOG(" destroying the pool: %d", id);
	cl_trpo_destroy(id);
	assert(alcnt1 == cl_allocing_cnt());


	LOG("6. Memory clear test");
	const size_t alcnt2 = cl_allocing_cnt();
	LOG(" alcnt1: %ld, alcnt2: %ld", alcnt1, alcnt2);
	assert(alcnt1 == alcnt2);

	DONE;
}

static void test_mem()
{
	int vmrss = 0, vmsize = 0, vmpeak = 0, vmdata = 0, vmstk = 0;
	Ret ret = cl_mem_get_curr_running_mems(&vmrss, &vmsize, &vmpeak, &vmdata, &vmstk);
	LOG("ret of get mem of self: %d", ret);
	LOG("\n  vmrss: %d\n  vmsize: %d\n  vmpeak: %d\n  vmdata: %d\n  vmstk: %d\n", vmrss, vmsize, vmpeak, vmdata, vmstk);
	assert(ret == SUCC);
	assert(vmrss > 0);
	assert(vmsize > 0);
	assert(vmpeak > 0);
	assert(vmdata > 0);
	assert(vmstk > 0);


	vmrss = 0;
	vmsize = 0;
	vmpeak = 0;
	vmdata = 0;
	vmstk = 0;
	ret = cl_mem_get_running_mems(1, &vmrss, &vmsize, &vmpeak, &vmdata, &vmstk);
	LOG("ret of get mem of 1: %d", ret);
	LOG("\n  vmrss: %d\n  vmsize: %d\n  vmpeak: %d\n  vmdata: %d\n  vmstk: %d\n", vmrss, vmsize, vmpeak, vmdata, vmstk);
	assert(ret == SUCC);
	assert(vmrss > 0);
	assert(vmsize > 0);
	assert(vmpeak > 0);
	assert(vmdata > 0);
	assert(vmstk > 0);

	DONE;
}

static void test_sys()
{
	test_mem();
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
	//test_thrdpol();
	//test_sys();
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
