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
#include <stdlib.h>
#include <string.h>

/* forward declarations */
static void test_qsort(void);

/* our test-module */
sTestModule tModQSort = {
	"qsort",
	&test_qsort
};

static int intCompare(const void *a,const void *b) {
	return *(const int*)a - *(const int*)b;
}

static int strCompare(const void *a,const void *b) {
	return strcmp(*(const char**)a,*(const char**)b);
}

static void test_qsort(void) {
	test_caseStart("Testing qsort");

	{
		int ints[] = {};
		qsort(ints,ARRAY_SIZE(ints),sizeof(int),intCompare);
	}

	{
		int ints[] = {1,1,1};
		qsort(ints,ARRAY_SIZE(ints),sizeof(int),intCompare);
		test_assertInt(ints[0],1);
		test_assertInt(ints[1],1);
		test_assertInt(ints[2],1);
	}

	{
		int ints[] = {1,2,3};
		qsort(ints,ARRAY_SIZE(ints),sizeof(int),intCompare);
		test_assertInt(ints[0],1);
		test_assertInt(ints[1],2);
		test_assertInt(ints[2],3);
	}

	{
		int ints[] = {6,7,3,4,2,1,5};
		qsort(ints,ARRAY_SIZE(ints),sizeof(int),intCompare);
		test_assertInt(ints[0],1);
		test_assertInt(ints[1],2);
		test_assertInt(ints[2],3);
		test_assertInt(ints[3],4);
		test_assertInt(ints[4],5);
		test_assertInt(ints[5],6);
		test_assertInt(ints[6],7);
	}

	{
		const char *strs[] = {
			"m3/m3-11.png",
			"m3/m3-03.png",
			"m3/m3-09.png",
			"m3/m3-20.png",
			"m3/m3-22.png",
			"m3/m3-17.png",
			"m3/m3-24.png",
			"m3/m3-12.png",
			"m3/m3-04.png",
			"m3/m3-23.png",
			"m3/m3-18.png",
			"m3/m3-01.png",
			"m3/m3-28.png",
			"m3/m3-14.png",
			"m3/m3-07.png",
			"m3/m3-26.png",
			"m3/m3-00.png",
			"m3/m3-05.png",
			"m3/m3-21.png",
			"m3/m3-16.png",
			"m3/m3-25.png",
			"m3/m3-08.png",
			"m3/m3-10.png",
			"m3/m3-02.png",
			"m3/m3-19.png",
			"m3/m3-06.png",
			"m3/m3-13.png",
			"m3/m3-15.png",
			"m3/m3-27.png",
		};
		qsort(strs,ARRAY_SIZE(strs),sizeof(char*),strCompare);
		for(size_t i = 0; i < ARRAY_SIZE(strs); ++i) {
			const char *sno = strs[i] + SSTRLEN("m3/m3-");
			int no = atoi(sno);
			test_assertInt(no,i);
		}
	}

	test_caseSucceeded();
}
