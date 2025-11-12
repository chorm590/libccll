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

TAG = "test";

#define DONE CLOGI("\e[32m%s\e[0m done", __FUNCTION__)

/******************************************
 **             common begin             **
 *****************************************/
static void test_alloc()
{
	char *buf1 = MALLOC(32);
	printf("addr of buf1: %p\n", buf1);
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
	CLOGD("freeing1 no: %d", dat1->no);
	FREE(dat1);
}

static void _tst_evt_dat2_free_fun(void *data)
{
	TRACE();
	if(data == NULL) return;

	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	_tst_evt_dat1_free_fun(dat2->dat1);
	CLOGD("freeing2 no: %d", dat2->no);
	FREE(dat2);
}

static void _tst_evt_dat3_free_fun(void *data)
{
	TRACE();
	if(data == NULL) return;

	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	CLOGD("freeing3 no: %d", dat3->no);
	FREE(dat3);
}

static Bool _tst_evt_cb1(uint16_t evt_no, void *data)
{
	TRACE();
	CLOGI("evt-no: %d", evt_no);
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) data;
	CLOGI("txt: %s, no: %d", dat1->txt, dat1->no);

	return false;
}

static Bool _tst_evt_cb2(uint16_t evt_no, void *data)
{
	TRACE();
	CLOGI("evt-no: %d", evt_no);
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) data;
	TST_EVT_DAT1 *dat1 = dat2->dat1;
	CLOGI("txt: %s, no: %d, no2: %d", dat1->txt, dat1->no, dat2->no);

	return false;
}

static Bool _tst_evt_cb3(uint16_t evt_no, void *data)
{
	TRACE();
	CLOGI("evt-no: %d", evt_no);
	TST_EVT_DAT3 *dat3 = (TST_EVT_DAT3 *) data;
	CLOGI("a: %s, no: %d", dat3->a, dat3->no);

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
	CLOGI("\n\ncase 1");
	TST_EVT_DAT1 *dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	Ret ret = cl_evt_pub(12, (void *) dat1, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-12: %d", ret);

	SLEEP(1);

	// 2. Just publish too
	CLOGI("\n\ncase 2");
	TST_EVT_DAT1 *dat1_2 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	ret = cl_evt_pub(13, (void *) dat1_2, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-13: %d", ret);

	SLEEP(1);

	// 3. Just publish too too
	CLOGI("\n\ncase 3");
	TST_EVT_DAT2 *dat2 = (TST_EVT_DAT2 *) MALLOC(sizeof(TST_EVT_DAT2));
	ret = cl_evt_pub(14, (void *) dat2, _tst_evt_dat2_free_fun);
	CLOGD("ret of pub evt-14: %d", ret);

	SLEEP(1);

	// 4. Sub and Pub
	CLOGI("\n\ncase 4");
	ret = cl_evt_sub(15, _tst_evt_cb1);
	CLOGD("ret of sub evt-15: %d", ret);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "Hello guy!");
	dat1->no = 185;
	ret = cl_evt_pub(15, (void *) dat1, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-15: %d", ret);
	SLEEP(1);
	cl_evt_unsub(15, _tst_evt_cb1);

	SLEEP(1);

	// 5. Single sub and twice publish
	CLOGI("\n\ncase 5");
	ret = cl_evt_sub(16, _tst_evt_cb1);
	CLOGD("ret of sub evt-16: %d", ret);
	dat1 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1->txt, "Msg 15, body her");
	dat1->no = 995;
	ret = cl_evt_pub(15, (void *) dat1, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-15: %d", ret);
	dat1_2 = (TST_EVT_DAT1 *) MALLOC(sizeof(TST_EVT_DAT1));
	sprintf(dat1_2->txt, "Msg 16, body her");
	dat1_2->no = 996;
	ret = cl_evt_pub(16, (void *) dat1_2, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-16: %d", ret);
	SLEEP(1);
	cl_evt_unsub(16, _tst_evt_cb1);
	dat1_2->txt[3] = 22;
	dat1_2->no = 998; // It should cause care-dump

	SLEEP(1);

	// 6. Multiply sub and multiple pub
	CLOGI("\n\ncase 6");
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
	CLOGI("\n\ncase 7");
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
	CLOGI("\n\ncase 8");
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
static void test_timer()
{
	TRACE();
	// 1.
	
	SLEEP(10);

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
	test_timer();
}

int main()
{
	if(cl_init(NULL) != SUCC)
	{
		printf("ccll init failed");
		return -1;
	}
	CLOGI("libccll init succ");

	test();

	cl_deinit();

	return 0;
}
