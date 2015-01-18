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
#include <sys/stat.h>
#include <sys/test.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* forward declarations */
static void test_dir(void);
static void test_opendir(void);
static void test_cleanpath(void);
static void test_basename(void);
static void test_dirname(void);

/* our test-module */
sTestModule tModDir = {
	"Dir",
	&test_dir
};

static void test_dir(void) {
	test_opendir();
	test_cleanpath();
	test_basename();
	test_dirname();
}

static void test_opendir(void) {
	DIR *dir;
	struct dirent e;
	test_caseStart("Testing opendir, readdir and closedir");

	dir = opendir("/bin");
	if(dir == NULL) {
		test_caseFailed("Unable to open '/bin'");
		return;
	}

	while(readdir(dir,&e));
	closedir(dir);

	test_caseSucceeded();
}

static void test_cleanpath(void) {
	size_t count;
	char *path;

	test_caseStart("Testing cleanpath");

	path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(path == NULL) {
		printe("Not enough mem for path");
		return;
	}

	setenv("CWD","/");

	count = cleanpath(path,MAX_PATH_LEN + 1,"/");
	test_assertUInt(count,SSTRLEN("/"));
	test_assertStr(path,"/");

	count = cleanpath(path,MAX_PATH_LEN + 1,"/bin/bla");
	test_assertUInt(count,SSTRLEN("/bin/bla"));
	test_assertStr(path,"/bin/bla");

	count = cleanpath(path,MAX_PATH_LEN + 1,"/../bin/../.././bla");
	test_assertUInt(count,SSTRLEN("/bla"));
	test_assertStr(path,"/bla");

	count = cleanpath(path,MAX_PATH_LEN + 1,"bin/..///.././bla");
	test_assertUInt(count,SSTRLEN("/bla"));
	test_assertStr(path,"/bla");

	count = cleanpath(path,MAX_PATH_LEN + 1,"bin/./bla");
	test_assertUInt(count,SSTRLEN("/bin/bla"));
	test_assertStr(path,"/bin/bla");

	setenv("CWD","/home");

	count = cleanpath(path,MAX_PATH_LEN + 1,"bin/./bla");
	test_assertUInt(count,SSTRLEN("/home/bin/bla"));
	test_assertStr(path,"/home/bin/bla");

	setenv("CWD","/home/");

	count = cleanpath(path,MAX_PATH_LEN + 1,"bin/./bla");
	test_assertUInt(count,SSTRLEN("/home/bin/bla"));
	test_assertStr(path,"/home/bin/bla");

	count = cleanpath(path,MAX_PATH_LEN + 1,"..");
	test_assertUInt(count,SSTRLEN("/"));
	test_assertStr(path,"/");

	count = cleanpath(path,MAX_PATH_LEN + 1,"../../.");
	test_assertUInt(count,SSTRLEN("/"));
	test_assertStr(path,"/");

	count = cleanpath(path,MAX_PATH_LEN + 1,"./../bin");
	test_assertUInt(count,SSTRLEN("/bin"));
	test_assertStr(path,"/bin");

	count = cleanpath(path,3,"/");
	if(count > 3)
		test_caseFailed("Copied too much");

	count = cleanpath(path,8,"/bin/bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	count = cleanpath(path,8,"/bin/../bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	count = cleanpath(path,8,"///../bin/bla");
	if(count > 8)
		test_caseFailed("Copied too much");

	free(path);

	test_caseSucceeded();
}

static void test_basename(void) {
	struct BasenameTest {
		const char *input;
		const char *expected;
	} tests[] = {
		{NULL,"."},
		{"","."},
		{"/","/"},
		{".","."},
		{"..",".."},
		{"a","a"},
		{"foo","foo"},
		{"/foo/bar","bar"},
		{"/foo/bar/","bar"},
		{"/foo/bar///","bar"},
	};

	test_caseStart("Testing basename");

	for(size_t i = 0; i < ARRAY_SIZE(tests); ++i) {
		char *cpy = tests[i].input ? strdup(tests[i].input) : NULL;
		test_assertTrue(tests[i].input == NULL || cpy != NULL);
		test_assertStr(basename(cpy),tests[i].expected);
		free(cpy);
	}

	test_caseSucceeded();
}

static void test_dirname(void) {
	struct DirnameTest {
		const char *input;
		const char *expected;
	} tests[] = {
		{NULL,"."},
		{"","."},
		{"/","/"},
		{".","."},
		{"..","."},
		{"a","."},
		{"foo","."},
		{"/foo/bar","/foo"},
		{"/foo/bar/","/foo"},
		{"/foo/bar///","/foo"},
		{"/a/","/"},
		{"abc///","."},
		{"abc//def///","abc"},
		{"abc/def/.","abc/def"},
	};

	test_caseStart("Testing dirname");

	for(size_t i = 0; i < ARRAY_SIZE(tests); ++i) {
		char *cpy = tests[i].input ? strdup(tests[i].input) : NULL;
		test_assertTrue(tests[i].input == NULL || cpy != NULL);
		test_assertStr(dirname(cpy),tests[i].expected);
		free(cpy);
	}

	test_caseSucceeded();
}
