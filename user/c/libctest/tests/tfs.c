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
#include <esc/io.h>
#include <esc/test.h>
#include <stdio.h>
#include <string.h>

static void test_fs(void);
static void fs_createFile(const char *name,const char *content);
static void fs_readFile(const char *name,const char *content);

sTestModule tModFs = {
	"FS",
	&test_fs
};

static void test_fs(void) {
	sFileInfo info1;
	sFileInfo info2;
	test_caseStart("Testing fs");

	/* don't try that on readonly-filesystems */
	stat("/bin",&info1);
	if(!(info1.mode & MODE_OWNER_WRITE)) {
		test_caseSucceded();
		return;
	}

	test_assertInt(mkdir("/newdir"),0);

	fs_createFile("/newdir/file1","foobar");
	fs_readFile("/newdir/file1","foobar");
	test_assertInt(link("/newdir/file1","/newdir/file2"),0);
	test_assertInt(stat("/newdir/file1",&info1),0);
	test_assertInt(stat("/newdir/file1",&info2),0);
	test_assertInt(memcmp(&info1,&info2,sizeof(sFileInfo)),0);
	test_assertUInt(info1.linkCount,2);
	test_assertInt(unlink("/newdir/file1"),0);
	test_assertInt(rmdir("/newdir"),ERR_DIR_NOT_EMPTY);
	test_assertInt(stat("/newdir/file1",&info1),ERR_PATH_NOT_FOUND);
	test_assertInt(stat("/newdir/file2",&info2),0);
	test_assertUInt(info2.linkCount,1);
	test_assertInt(unlink("/newdir/file2"),0);

	test_assertInt(rmdir("/newdir"),0);
	test_assertInt(sync(),0);

	test_caseSucceded();
}

static void fs_createFile(const char *name,const char *content) {
	FILE *f = fopen(name,"w");
	test_assertTrue(f != NULL);
	test_assertInt(fwrite(content,1,strlen(content),f),strlen(content));
	fclose(f);
}

static void fs_readFile(const char *name,const char *content) {
	char buf[100] = {0};
	FILE *f = fopen(name,"r");
	test_assertTrue(f != NULL);
	test_assertInt(fseek(f,0,SEEK_END),0);
	test_assertInt(ftell(f),strlen(content));
	rewind(f);
	test_assertInt(fread(buf,1,strlen(content),f),strlen(content));
	test_assertStr(buf,content);
	fclose(f);
}
