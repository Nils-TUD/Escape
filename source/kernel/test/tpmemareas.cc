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
#include <sys/mem/pmemareas.h>
#include <sys/util.h>
#include <esc/test.h>
#include "tmm.h"
#include "testutils.h"

/* forward declarations */
static void test_pmemareas(void);

/* our test-module */
sTestModule tModPmemAreas = {
	"PhysMem Areas",
	&test_pmemareas
};

static PhysMemAreas::MemArea backup[16];

static void test_pmemareas(void) {
	size_t i,available,count = 0;
	const PhysMemAreas::MemArea *area;
	test_caseStart("Adding and removing areas");

	/* backup current areas and remove all */
	for(i = 0, area = PhysMemAreas::get(); area != NULL; ++i, area = area->next) {
		if(i >= ARRAY_SIZE(backup))
			util_panic("Out of backup-slots for physmem-areas");
		backup[i].addr = area->addr;
		backup[i].size = area->size;
		count++;
	}
	available = PhysMemAreas::getAvailable();
	while((area = PhysMemAreas::get()))
		PhysMemAreas::rem(area->addr,area->addr + area->size);

	/* add some test areas */
	PhysMemAreas::add(0x1000,0x3000);
	PhysMemAreas::add(0x5000,0x8000);
	PhysMemAreas::add(0x8000,0xD000);
	PhysMemAreas::add(0x4000,0x5000);
	test_assertSize(PhysMemAreas::getAvailable(),0xB000);

	/* rem end */
	PhysMemAreas::rem(0x7000,0x8000);
	test_assertSize(PhysMemAreas::getAvailable(),0xA000);

	/* rem nothing */
	PhysMemAreas::rem(0x0000,0x1000);
	test_assertSize(PhysMemAreas::getAvailable(),0xA000);

	/* rem middle */
	PhysMemAreas::rem(0x9000,0xA000);
	test_assertSize(PhysMemAreas::getAvailable(),0x9000);

	/* rem begin */
	PhysMemAreas::rem(0x5000,0x6000);
	test_assertSize(PhysMemAreas::getAvailable(),0x8000);

	/* rem complete area */
	PhysMemAreas::rem(0x4000,0x5000);
	test_assertSize(PhysMemAreas::getAvailable(),0x7000);

	/* add something again */
	PhysMemAreas::add(0x4000,0x5000);
	test_assertSize(PhysMemAreas::getAvailable(),0x8000);

	/* remove all */
	PhysMemAreas::rem(0x0000,0xD000);
	test_assertSize(PhysMemAreas::getAvailable(),0);

	/* restore old state */
	for(i = 0; i < count; ++i)
		PhysMemAreas::add(backup[i].addr,backup[i].addr + backup[i].size);
	test_assertSize(PhysMemAreas::getAvailable(),available);

	test_caseSucceeded();
}
