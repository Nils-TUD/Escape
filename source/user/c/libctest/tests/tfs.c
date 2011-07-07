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
#include <esc/proc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_fs(void);
static void test_basics(void);
static void test_perms(void);
static void test_assertCan(const char *path,uint mode);
static void test_assertCanNot(const char *path,uint mode,int err);
static void fs_createFile(const char *name,const char *content);
static void fs_readFile(const char *name,const char *content);

sTestModule tModFs = {
	"FS",
	&test_fs
};

static void test_fs(void) {
	sFileInfo info;
	/* don't try that on readonly-filesystems */
	if(stat("/bin",&info) < 0)
		error("Unable to stat /bin");
	if(!(info.mode & S_IWUSR))
		return;

	test_basics();
	test_perms();
}

static void test_basics(void) {
	sFileInfo info1;
	sFileInfo info2;
	test_caseStart("Testing fs");

	test_assertInt(mkdir("/newdir"),0);

	fs_createFile("/newdir/file1","foobar");
	fs_readFile("/newdir/file1","foobar");
	test_assertInt(link("/newdir/file1","/newdir/file2"),0);
	test_assertInt(stat("/newdir/file1",&info1),0);
	test_assertInt(stat("/newdir/file2",&info2),0);
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

	test_caseSucceeded();
}

static void test_perms(void) {
	size_t i;
	sFileInfo info;
	struct {
		const char *dir;
		const char *file;
	} paths[] = {
		{"/newfile","/newfile/test"},
		{"/system/newfile","/system/newfile/test"}
	};
	test_caseStart("Testing permissions");

	for(i = 0; i < ARRAY_SIZE(paths); i++) {
		/* create new file */
		fs_createFile(paths[i].dir,"foobar");
		test_assertInt(chmod(paths[i].dir,0600),0);
		test_assertInt(chown(paths[i].dir,1,1),0);

		/* I'm the owner */
		test_assertInt(setegid(1),0);
		test_assertInt(seteuid(1),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCan(paths[i].dir,IO_WRITE);

		/* I'm NOT the owner */
		test_assertInt(seteuid(0),0);
		test_assertInt(seteuid(2),0);

		test_assertCanNot(paths[i].dir,IO_READ,ERR_NO_READ_PERM);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);

		/* give group read-perm */
		test_assertInt(seteuid(0),0);
		test_assertInt(chmod(paths[i].dir,0640),0);
		test_assertInt(seteuid(2),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);

		/* neither owner nor group */
		test_assertInt(seteuid(0),0);
		test_assertInt(setegid(0),0);
		test_assertInt(setegid(2),0);
		test_assertInt(seteuid(2),0);

		test_assertCanNot(paths[i].dir,IO_READ,ERR_NO_READ_PERM);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);

		/* give others read+write perm */
		test_assertInt(seteuid(0),0);
		test_assertInt(chmod(paths[i].dir,0646),0);
		test_assertInt(seteuid(2),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCan(paths[i].dir,IO_WRITE);

		/* delete it */
		test_assertInt(seteuid(0),0);
		test_assertInt(unlink(paths[i].dir),0);


		/* create new folder */
		test_assertInt(mkdir(paths[i].dir),0);
		test_assertInt(chmod(paths[i].dir,0700),0);
		test_assertInt(chown(paths[i].dir,1,1),0);

		/* I'm the owner */
		test_assertInt(setegid(1),0);
		test_assertInt(seteuid(1),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCan(paths[i].dir,IO_WRITE);
		fs_createFile(paths[i].file,"foo");
		test_assertInt(stat(paths[i].file,&info),0);

		/* I'm NOT the owner */
		test_assertInt(seteuid(0),0);
		test_assertInt(seteuid(2),0);

		test_assertCanNot(paths[i].dir,IO_READ,ERR_NO_READ_PERM);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);
		test_assertInt(stat(paths[i].file,&info),ERR_NO_EXEC_PERM);

		/* give group read-perm */
		test_assertInt(seteuid(0),0);
		test_assertInt(chmod(paths[i].dir,0740),0);
		test_assertInt(seteuid(2),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);
		test_assertInt(stat(paths[i].file,&info),ERR_NO_EXEC_PERM);

		/* neither owner nor group */
		test_assertInt(seteuid(0),0);
		test_assertInt(setegid(0),0);
		test_assertInt(setegid(2),0);
		test_assertInt(seteuid(2),0);

		test_assertCanNot(paths[i].dir,IO_READ,ERR_NO_READ_PERM);
		test_assertCanNot(paths[i].dir,IO_WRITE,ERR_NO_WRITE_PERM);
		test_assertInt(stat(paths[i].file,&info),ERR_NO_EXEC_PERM);

		/* give others read+write perm */
		test_assertInt(seteuid(0),0);
		test_assertInt(chmod(paths[i].dir,0747),0);
		test_assertInt(seteuid(2),0);

		test_assertCan(paths[i].dir,IO_READ);
		test_assertCan(paths[i].dir,IO_WRITE);
		test_assertInt(stat(paths[i].file,&info),0);

		/* delete it */
		test_assertInt(seteuid(0),0);
		test_assertInt(unlink(paths[i].file),0);
		test_assertInt(rmdir(paths[i].dir),0);
	}

	test_caseSucceeded();
}

static void test_assertCan(const char *path,uint mode) {
	int fd = open(path,mode);
	test_assertTrue(fd >= 0);
	close(fd);
}

static void test_assertCanNot(const char *path,uint mode,int err) {
	test_assertInt(open(path,mode),err);
}

static void fs_createFile(const char *name,const char *content) {
	FILE *f = fopen(name,"w");
	test_assertTrue(f != NULL);
	if(f != NULL) {
		test_assertInt(fwrite(content,1,strlen(content),f),strlen(content));
		fclose(f);
	}
}

static void fs_readFile(const char *name,const char *content) {
	char buf[100] = {0};
	FILE *f = fopen(name,"r");
	test_assertTrue(f != NULL);
	if(f != NULL) {
		test_assertInt(fseek(f,0,SEEK_END),0);
		test_assertInt(ftell(f),strlen(content));
		rewind(f);
		test_assertInt(fread(buf,1,strlen(content),f),strlen(content));
		test_assertStr(buf,content);
		fclose(f);
	}
}
