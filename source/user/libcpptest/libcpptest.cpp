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
#include <sys/test.h>
#include <stdio.h>

extern sTestModule tModString;
extern sTestModule tModVector;
extern sTestModule tModList;
extern sTestModule tModAlgo;
extern sTestModule tModStreams;
extern sTestModule tModFunctional;
extern sTestModule tModBintree;
extern sTestModule tModMap;
extern sTestModule tModCmdArgs;
extern sTestModule tModSmartPtr;
extern sTestModule tModTuple;

int main(void) {
	test_register(&tModString);
	test_register(&tModVector);
	test_register(&tModList);
	test_register(&tModAlgo);
	test_register(&tModStreams);
	test_register(&tModFunctional);
	test_register(&tModBintree);
	test_register(&tModMap);
	test_register(&tModCmdArgs);
	test_register(&tModSmartPtr);
	test_register(&tModTuple);
	test_start();
	/* flush stdout because cout will be closed before stdout is flushed by exit(). thus, that flush
	 * will fail because the file has already been closed. */
	fflush(stdout);
	return EXIT_SUCCESS;
}
