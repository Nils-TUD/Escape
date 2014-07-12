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
#include <dirent.h>
#include <sys/test.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* forward declarations */
static void test_setjmp(void);

/* our test-module */
sTestModule tModSetjmp = {
	"setjmp",
	&test_setjmp
};

static jmp_buf env;

#if defined(__x86__)
static void myfunc(int i) {
	if(i > 0)
		myfunc(i - 1);
	else
		longjmp(env,1);
}
#endif

static void test_setjmp(void) {
	test_caseStart("Basic setjmp and longjmp");

	/* not supported on eco32 and mmix yet */
#if defined(__x86__)
	int res = setjmp(env);
	if(res == 0) {
		myfunc(5);
		test_assertFalse(true);
	}
	test_assertInt(res, 1);
#endif

	test_caseSucceeded();
}
