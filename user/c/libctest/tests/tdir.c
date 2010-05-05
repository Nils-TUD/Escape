/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/heap.h>
#include <stdio.h>
#include <test.h>
#include <stdarg.h>
#include <string.h>
#include "tdir.h"

/* forward declarations */
static void test_dir(void);
static void test_opendir(void);
static void test_abspath(void);

/* our test-module */
sTestModule tModDir = {
	"Dir",
	&test_dir
};

static void test_dir(void) {
	test_opendir();
	test_abspath();
}

static void test_opendir(void) {
	tFD fd;
	sDirEntry e;
	test_caseStart("Testing opendir, readdir and closedir");

	fd = opendir("/bin");
	if(fd < 0) {
		test_caseFailed("Unable to open '/bin'");
		return;
	}

	while(readdir(&e,fd));
	closedir(fd);

	test_caseSucceded();
}

static void test_abspath(void) {
	u32 count;
	char *path;

	test_caseStart("Testing abspath");

	path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(path == NULL) {
		printe("Not enough mem for path");
		return;
	}

	count = abspath(path,MAX_PATH_LEN + 1,"/");
	test_assertUInt(count,SSTRLEN("/"));
	test_assertStr(path,"/");

	count = abspath(path,MAX_PATH_LEN + 1,"/bin/bla");
	test_assertUInt(count,SSTRLEN("/bin/bla/"));
	test_assertStr(path,"/bin/bla/");

	count = abspath(path,MAX_PATH_LEN + 1,"/../bin/../.././bla");
	test_assertUInt(count,SSTRLEN("/bla/"));
	test_assertStr(path,"/bla/");

	count = abspath(path,MAX_PATH_LEN + 1,"bin/..///.././bla");
	test_assertUInt(count,SSTRLEN("/bla/"));
	test_assertStr(path,"/bla/");

	count = abspath(path,MAX_PATH_LEN + 1,"bin/./bla");
	test_assertUInt(count,SSTRLEN("/bin/bla/"));
	test_assertStr(path,"/bin/bla/");

	count = abspath(path,3,"/");
	if(count > 3)
		test_caseFailed("Copied too much");

	count = abspath(path,8,"/bin/bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	count = abspath(path,8,"/bin/../bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	count = abspath(path,8,"///../bin/bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	free(path);

	test_caseSucceded();
}
