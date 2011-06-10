/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/dynarray.h>
#include <esc/test.h>
#include "tdynarray.h"

/* use the sllnode-area here and start a bit above to be sure that its not used yet */
#define REGION_START		(SLLNODE_AREA + PAGE_SIZE * 8)
#define REGION_SIZE			(PAGE_SIZE * 8)

/* forward declarations */
static void test_dynarray(void);

/* our test-module */
sTestModule tModDynArray = {
	"Dynamic array",
	&test_dynarray
};

static void test_dynarray(void) {
	test_caseStart("Test various functions");

	size_t i,j;
	size_t mappedPages = paging_dbg_getPageCount();
	size_t freeFrames = pmem_getFreeFrames(MM_DEF | MM_CONT);
	sDynArray da;
	dyna_start(&da,sizeof(uint),REGION_START,REGION_SIZE);

	for(j = 0; j < REGION_SIZE; j += PAGE_SIZE) {
		size_t oldCount = da.objCount;
		test_assertTrue(dyna_extend(&da));
		for(i = oldCount; i < da.objCount; i++) {
			uint *l = dyna_getObj(&da,i);
			*l = i;
		}
	}

	test_assertFalse(dyna_extend(&da));

	for(i = 0, j = 0; j < REGION_SIZE; i++, j += sizeof(uint)) {
		uint *l = dyna_getObj(&da,i);
		test_assertUInt(dyna_getIndex(&da,l),i);
		test_assertUInt(*l,i);
	}

	dyna_destroy(&da);

	test_assertUInt(paging_dbg_getPageCount(),mappedPages);
	test_assertUInt(pmem_getFreeFrames(MM_DEF | MM_CONT),freeFrames);

	test_caseSucceeded();
}
