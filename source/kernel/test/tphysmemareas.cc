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

#include <common.h>
#include <mem/physmemareas.h>
#include <util.h>
#include <sys/test.h>
#include "testutils.h"

/* forward declarations */
static void test_pmemareas();

/* our test-module */
sTestModule tModPmemAreas = {
	"PhysMem Areas",
	&test_pmemareas
};

static PhysMemAreas::MemArea backup[16];

static void test_pmemareas() {
	size_t i,available,count = 0;
	const PhysMemAreas::MemArea *area;
	test_caseStart("Adding and removing areas");

	/* backup current areas and remove all */
	for(i = 0, area = PhysMemAreas::get(); area != NULL; ++i, area = area->next) {
		if(i >= ARRAY_SIZE(backup))
			Util::panic("Out of backup-slots for physmem-areas");
		backup[i].addr = area->addr;
		backup[i].size = area->size;
		count++;
	}
	available = PhysMemAreas::getAvailable();
	while((area = PhysMemAreas::get()))
		PhysMemAreas::rem(area->addr,area->addr + area->size);

	/* add some test areas */
	PhysMemAreas::add(PAGE_SIZE * 1,PAGE_SIZE * 3);
	PhysMemAreas::add(PAGE_SIZE * 5,PAGE_SIZE * 8);
	PhysMemAreas::add(PAGE_SIZE * 8,PAGE_SIZE * 13);
	PhysMemAreas::add(PAGE_SIZE * 4,PAGE_SIZE * 5);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 11);

	/* rem end */
	PhysMemAreas::rem(PAGE_SIZE * 7,PAGE_SIZE * 8);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 10);

	/* rem nothing */
	PhysMemAreas::rem(PAGE_SIZE * 0,PAGE_SIZE * 1);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 10);

	/* rem middle */
	PhysMemAreas::rem(PAGE_SIZE * 9,PAGE_SIZE * 10);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 9);

	/* rem begin */
	PhysMemAreas::rem(PAGE_SIZE * 5,PAGE_SIZE * 6);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 8);

	/* rem complete area */
	PhysMemAreas::rem(PAGE_SIZE * 4,PAGE_SIZE * 5);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 7);

	/* add something again */
	PhysMemAreas::add(PAGE_SIZE * 4,PAGE_SIZE * 5);
	test_assertSize(PhysMemAreas::getAvailable(),PAGE_SIZE * 8);

	/* remove all */
	PhysMemAreas::rem(PAGE_SIZE * 0,PAGE_SIZE * 13);
	test_assertSize(PhysMemAreas::getAvailable(),0);

	/* restore old state */
	for(i = 0; i < count; ++i)
		PhysMemAreas::add(backup[i].addr,backup[i].addr + backup[i].size);
	test_assertSize(PhysMemAreas::getAvailable(),available);

	test_caseSucceeded();
}
