/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sllist.h"
#include "../h/kheap.h"
#include "../h/video.h"

#include "tsllist.h"
#include "test.h"

/* forward declarations */
static void test_sllist(void);
static void test_1(void);
static void test_2(void);
static void test_3(void);
static void test_4(void);
static void test_5(void);
static void test_6(void);
static void test_7(void);
static void test_8(void);

/* our test-module */
sTestModule tModSLList = {
	"Single linked list",
	&test_sllist
};

static void test_sllist(void) {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
	test_6();
	test_7();
	test_8();
}

static void test_1(void) {
	u32 i,free,x = 0x100,len;
	bool res = true;
	sSLList *list;
	free = kheap_getFreeMem();
	test_caseStart("Append & check & remove index 0");

	list = sll_create();
	for(i = 0; i < 20; i++) {
		sll_append(list,(void*)x++);
	}
	x = 0x100;
	for(i = 0; i < 20; i++) {
		if(sll_get(list,i) != (void*)x++) {
			res = false;
			break;
		}
	}
	if(res) {
		for(i = 0; i < 20; i++) {
			sll_removeIndex(list,0);
		}
	}

	len = sll_length(list);
	sll_destroy(list);
	if(!res || kheap_getFreeMem() != free || len != 0)
		test_caseFailed("Got wrong element, elements not removed or memory not freed");
	else
		test_caseSucceded();
}

static void test_2(void) {
	u32 i,free,x = 0x100,len;
	sSLList *list;
	free = kheap_getFreeMem();
	test_caseStart("Append & remove first (NULL)");

	list = sll_create();
	for(i = 0; i < 2; i++) {
		sll_append(list,(void*)x++);
	}
	for(i = 0; i < 2; i++) {
		sll_removeFirst(list,NULL);
	}

	len = sll_length(list);
	sll_destroy(list);
	if(kheap_getFreeMem() != free || len != 0)
		test_caseFailed("Elements not removed or memory not freed");
	else
		test_caseSucceded();
}

static void test_3(void) {
	u32 i,free,x = 0x100,len;
	sSLList *list;
	free = kheap_getFreeMem();
	test_caseStart("Append & remove first (x)");

	list = sll_create();
	for(i = 0; i < 50; i++) {
		sll_append(list,(void*)x++);
	}
	x = 0x100;
	for(i = 0; i < 50; i++) {
		sll_removeFirst(list,(void*)x++);
	}

	len = sll_length(list);
	sll_destroy(list);
	if(kheap_getFreeMem() != free || len != 0)
		test_caseFailed("Elements not removed or memory not freed");
	else
		test_caseSucceded();
}

static void test_4(void) {
	u32 i,free,x = 0x100;
	sSLList *list;
	free = kheap_getFreeMem();
	test_caseStart("Create & append & destroy");

	list = sll_create();
	for(i = 0; i < 200; i++) {
		sll_append(list,(void*)x++);
	}
	sll_destroy(list);

	if(kheap_getFreeMem() != free)
		test_caseFailed("Memory not freed");
	else
		test_caseSucceded();
}

static void test_5(void) {
	u32 i,free,x = 0x100;
	sSLList *list;
	bool res = true;
	free = kheap_getFreeMem();
	test_caseStart("Create & append & insert somewhere & destroy");

	list = sll_create();
	for(i = 0; i < 5; i++) {
		sll_append(list,(void*)x++);
	}
	sll_insert(list,(void*)0x200,3);
	if(sll_get(list,3) != (void*)0x200)
		res = false;
	sll_insert(list,(void*)0x201,2);
	if(sll_get(list,2) != (void*)0x201)
		res = false;
	sll_insert(list,(void*)0x202,1);
	if(sll_get(list,1) != (void*)0x202)
		res = false;
	sll_insert(list,(void*)0x203,0);
	if(sll_get(list,0) != (void*)0x203)
		res = false;
	sll_insert(list,(void*)0x204,7);
	if(sll_get(list,7) != (void*)0x204)
		res = false;
	if(sll_length(list) != 10)
		res = false;
	sll_destroy(list);

	if(!res || kheap_getFreeMem() != free)
		test_caseFailed("Insert wrong or memory not freed");
	else
		test_caseSucceded();
}

static void test_6(void) {
	u32 i,free,x = 0x100;
	sSLList *list;
	bool res = true;
	free = kheap_getFreeMem();
	test_caseStart("Create & append & set somewhere & destroy");

	list = sll_create();
	for(i = 0; i < 5; i++) {
		sll_append(list,(void*)x++);
	}
	sll_set(list,(void*)0x200,3);
	if(sll_get(list,3) != (void*)0x200)
		res = false;
	sll_set(list,(void*)0x201,2);
	if(sll_get(list,2) != (void*)0x201)
		res = false;
	sll_set(list,(void*)0x202,1);
	if(sll_get(list,1) != (void*)0x202)
		res = false;
	sll_set(list,(void*)0x203,0);
	if(sll_get(list,0) != (void*)0x203)
		res = false;
	sll_set(list,(void*)0x204,4);
	if(sll_get(list,4) != (void*)0x204)
		res = false;
	if(sll_length(list) != 5)
		res = false;
	sll_destroy(list);

	if(!res || kheap_getFreeMem() != free)
		test_caseFailed("Set wrong or Memory not freed");
	else
		test_caseSucceded();
}

static void test_7(void) {
	u32 free,x = 0x100;
	bool res;
	sSLList *list;
	free = kheap_getFreeMem();
	test_caseStart("Append until no mem left & destroy");

	list = sll_create();
	do {
		res = sll_append(list,(void*)x++);
	}
	while(res);
	tprintf("Appended %d elements\n",sll_length(list));
	tprintf("Freeing...");
	sll_destroy(list);
	tprintf("done\n");

	if(kheap_getFreeMem() != free)
		test_caseFailed("Memory not freed (before=%d, after=%d)",free,kheap_getFreeMem());
	else
		test_caseSucceded();
}

static void test_8(void) {
	sSLList *list;
	sSLNode *n;

	test_caseStart("Walking through the list");

	list = sll_create();
	sll_append(list,(void*)0x123);
	sll_append(list,(void*)0x456);
	sll_append(list,(void*)0x789);

	tprintf("Starting at index 0\n");
	for(n = sll_begin(list); n != NULL; n = n->next) {
		tprintf("element @ 0x%x : 0x%x\n",n,n->data);
	}
	tprintf("Starting at index 2\n");
	for(n = sll_nodeAt(list,2); n != NULL; n = n->next) {
		tprintf("element @ 0x%x : 0x%x\n",n,n->data);
	}
	sll_destroy(list);

	test_caseSucceded();
}
