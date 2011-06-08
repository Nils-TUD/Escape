/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/arch/mmix/mem/addrspace.h>
#include <esc/test.h>

#define ADDR_SPACE_COUNT		1024

/* forward declarations */
static void test_addrspace(void);
static void test_basics(void);
static void test_dupUsage(void);

/* our test-module */
sTestModule tModAddrSpace = {
	"Address space",
	&test_addrspace
};
static sAddressSpace *spaces[ADDR_SPACE_COUNT * 3];

static void test_addrspace(void) {
	test_basics();
	test_dupUsage();
	/* do it twice to ensure that the cleanup is correct */
	test_dupUsage();
}

static void test_basics(void) {
	sAddressSpace *as1,*as2;
	test_caseStart("Testing basics");

	as1 = aspace_alloc();
	as2 = aspace_alloc();
	test_assertTrue(as1 != as2);
	test_assertTrue(as1->no != as2->no);
	test_assertUInt(as1->refCount,1);
	test_assertUInt(as2->refCount,1);

	aspace_free(as2);
	aspace_free(as1);

	test_caseSucceeded();
}

static void test_dupUsage(void) {
	size_t i;
	test_caseStart("Testing duplicate usage");

	/* allocate 0 .. 1023 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[i]->refCount,1);
	}

	/* allocate 1024 .. 2047 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT + i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->refCount,2);
	}

	/* allocate 2048 .. 3071 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT * 2 + i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->refCount,3);
	}

	/* free 3071 .. 2048 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[ADDR_SPACE_COUNT * 2 + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->refCount,2);
	}
	/* free 2047 .. 1024 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[ADDR_SPACE_COUNT + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->refCount,1);
	}
	/* free 2047 .. 0 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[i]);
		test_assertUInt(spaces[i]->refCount,0);
	}

	test_caseSucceeded();
}
