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

static void test_list()
{
	typedef struct {
		int id;
		char name[16];
		int age;
		LIST_HEAD list;
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
		init_list_head(&emp->list);

		list_add(&emp->list, &emps);
		assert(list_empty(&emps) == false);
	}
	// Check the size of list
	assert(list_size(&emps) == S);
	// Check the value
	LIST_HEAD *node = &emps;
	for(i = 0; i < S; i++)
	{
		LIST_HEAD *a = node->next;
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
		LIST_HEAD *a = node->next;
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

typedef struct {
	char txt[18];
	int num;
} Tst_Evt_Dat1;

static void _tst_evt_dat1_free_fun(void *data)
{
	TRACE();
	if(data == NULL) return;

	Tst_Evt_Dat1 *dat1 = (Tst_Evt_Dat1 *) data;
	FREE(dat1);
}

static void test_event()
{
	TRACE();
	cl_iter_objs();
	Tst_Evt_Dat1 *dat1 = (Tst_Evt_Dat1 *) MALLOC(sizeof(Tst_Evt_Dat1));
	Ret ret = cl_evt_pub(12, (void *) dat1, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-12: %d", ret);
	SLEEP(1);
	Tst_Evt_Dat1 *dat1_2 = (Tst_Evt_Dat1 *) MALLOC(sizeof(Tst_Evt_Dat1));
	ret = cl_evt_pub(13, (void *) dat1_2, _tst_evt_dat1_free_fun);
	CLOGD("ret of pub evt-13: %d", ret);
	SLEEP(2);
	cl_iter_objs();
	DONE;
}

static void test()
{
	//test_common();
	//test_queue();
	test_event();
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
