#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cl_def.h"
#include "cl_ccll.h"
#include "cl_log.h"
#include "cl_alloc.h"
#include "cl_list.h"

TAG = "test";

#define DONE CLOGI("\e[32m%s\e[0m done", __FUNCTION__)

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
	for(i = 0; i < 100; i++)
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



	DONE;
}

static void test_queue()
{
	test_list();
}

static void test()
{
	test_queue();
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
