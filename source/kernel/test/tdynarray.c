/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/dynarray.h>
#include <esc/test.h>
#include "tdynarray.h"
#include "testutils.h"

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
	size_t i,j;
	sDynArray da;
	test_caseStart("Test various functions");

	checkMemoryBefore(true);
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
		test_assertSSize(dyna_getIndex(&da,l),i);
		test_assertUInt(*l,i);
	}

	dyna_destroy(&da);
	checkMemoryAfter(true);

	test_caseSucceeded();
}
