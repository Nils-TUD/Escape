/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <sys/test.h>
#include <common.h>

#include "testutils.h"

/* use the sllnode-area here and start a bit above to be sure that its not used yet */
#define REGION_START		(SLLNODE_AREA + PAGE_SIZE * 8)
#define REGION_SIZE			(PAGE_SIZE * 8)

/* forward declarations */
static void test_dynarray();

/* our test-module */
sTestModule tModDynArray = {
	"Dynamic array",
	&test_dynarray
};

static void test_dynarray() {
	test_caseStart("Test various functions");

	checkMemoryBefore(true);
	{
		DynArray da(sizeof(uint),REGION_START,REGION_SIZE);

		for(size_t j = 0; j < REGION_SIZE; j += PAGE_SIZE) {
			size_t oldCount = da.getObjCount();
			test_assertTrue(da.extend());
			for(size_t i = oldCount; i < da.getObjCount(); i++) {
				uint *l = (uint*)da.getObj(i);
				*l = i;
			}
		}

		test_assertFalse(da.extend());

		for(size_t i = 0, j = 0; j < REGION_SIZE; i++, j += sizeof(uint)) {
			uint *l = (uint*)da.getObj(i);
			test_assertSSize(da.getIndex(l),i);
			test_assertUInt(*l,i);
		}
	}
	checkMemoryAfter(true);

	test_caseSucceeded();
}
