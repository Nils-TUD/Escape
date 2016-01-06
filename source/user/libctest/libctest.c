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

#include <sys/common.h>
#include <sys/proc.h>
#include <sys/test.h>
#include <stdlib.h>

extern sTestModule tModHeap;
extern sTestModule tModFileio;
extern sTestModule tModDir;
extern sTestModule tModEnv;
extern sTestModule tModFs;
extern sTestModule tModMem;
extern sTestModule tModSLList;
extern sTestModule tModSetjmp;
extern sTestModule tModString;
extern sTestModule tModMath;
extern sTestModule tModQSort;

int main(void) {
	if(getuid() != ROOT_UID)
		error("Please start this program as root!");

	test_register(&tModHeap);
	test_register(&tModFileio);
	test_register(&tModDir);
	test_register(&tModEnv);
	test_register(&tModFs);
	test_register(&tModMem);
	test_register(&tModSLList);
	test_register(&tModSetjmp);
	test_register(&tModString);
	test_register(&tModMath);
	test_register(&tModQSort);
	test_start();
	return EXIT_SUCCESS;
}
