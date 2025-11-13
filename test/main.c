#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "cl_def.h"
#include "cl_ccll.h"
#include "cl_log.h"
#include "cl_alloc.h"
#include "cl_list.h"
#include "cl_event.h"
#include "cl_wait.h"
#include "cl_timer.h"

TAG = "test";

#define DONE printf("\e[32m%s\e[0m done\n", __FUNCTION__)
#define LOG(fmt, args...) \
	printf("\e[36m"); \
	printf(fmt, ##args); \
	printf("\e[0m\n")

/******************************************
 **             common begin             **
 *****************************************/
static void test_alloc()
{
	char *buf1 = MALLOC(32);
	LOG("addr of buf1: %p", buf1);
	cl_iter_objs();
	FREE(buf1);
	cl_iter_objs();
}

static void test_common()
{
	test_alloc();
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
	TRACE();
	if(data == NULL) return;

	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	LOG("freeing1 no: %d", dat1->no);
	FREE(dat1);
}

static void _tst_evt_dat2_free_fun(void *data)
{
	TRACE();
	if(data == NULL) return;

	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	_tst_evt_dat1_free_fun(dat2->dat1);
	LOG("freeing2 no: %d", dat2->no);
	FREE(dat2);
}

static void _tst_evt_dat3_free_fun(void *data)
{
	TRACE();
	if(data == NULL) return;

	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	LOG("freeing3 no: %d", dat3->no);
	FREE(dat3);
}

static Bool _tst_evt_cb1(uint16_t evt_no, void *data)
{
	TRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	LOG("txt: %s, no: %d", dat1->txt, dat1->no);

	return false;
}

static Bool _tst_evt_cb2(uint16_t evt_no, void *data)
{
	TRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	TST_EVT_DAT1 *dat1 = dat2->dat1;
	LOG("txt: %s, no: %d, no2: %d", dat1->txt, dat1->no, dat2->no);

	return false;
}

static Bool _tst_evt_cb3(uint16_t evt_no, void *data)
{
	TRACE();
	LOG("evt-no: %d", evt_no);
	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	LOG("a: %s, no: %d", dat3->a, dat3->no);

	return false;
}

static Bool _tst_evt_cb20(uint16_t evt_no, void *data)
{
	TRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb21(uint16_t evt_no, void *data)
{
	TRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb22(uint16_t evt_no, void *data)
{
	TRACE();
	return _tst_evt_cb1(evt_no, data);
}

static Bool _tst_evt_cb23(uint16_t evt_no, void *data)
{
	TRACE();
	_tst_evt_cb1(evt_no, data);
	return true;
}

static void test_event()
{
	TRACE();

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
	TRACE();
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
 **             testing menu             **
 ******************************************/
static void test()
{
	//test_common();
	//test_queue();
	//test_event();
	//test_timer();
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
