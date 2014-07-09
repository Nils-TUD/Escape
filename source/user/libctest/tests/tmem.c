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

#include <esc/common.h>
#include <esc/test.h>
#include <esc/mem.h>
#include <esc/elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* forward declarations */
static void test_mem(void);
static void test_mmap_file(void);
static void test_mmap_shared_file(const char *path);

/* our test-module */
sTestModule tModMem = {
	"Memory",
	&test_mem
};

static void test_mem(void) {
	test_mmap_file();
	test_mmap_shared_file("/sys/foobar");
	sFileInfo info;
	if(stat(".",&info) < 0)
		error("Unable to stat .");
	/* don't try that on readonly-filesystems */
	if((info.mode & S_IWUSR))
		test_mmap_shared_file("foobar");
}

static void test_mmap_file(void) {
	void *addr;
	sElfEHeader *header;

	test_caseStart("Testing mmap() with a file");

	int fd = open("/bin/libctest",O_RDONLY);
	if(fd < 0) {
		test_assertFalse(true);
		return;
	}
	addr = mmap(NULL,8192,8192,PROT_READ,MAP_PRIVATE,fd,0);
	if(!addr) {
		test_assertFalse(true);
		return;
	}
	header = (sElfEHeader*)addr;
	test_assertUInt(header->e_ident[0],'\x7F');
	test_assertUInt(header->e_ident[1],'E');
	test_assertUInt(header->e_ident[2],'L');
	test_assertUInt(header->e_ident[3],'F');
	munmap(addr);

	test_caseSucceeded();
}

static void test_mmap_shared_file(const char *path) {
	void *addr;
	size_t i;
	FILE *f;
	int fd;

	test_caseStart("Testing mmap() of '%s' with write-back",path);

	/* create file with test content */
	f = fopen(path,"w+");
	if(!f) {
		test_assertFalse(true);
		return;
	}
	for(i = 0; i < 9000; ++i)
		fputc('0' + i % 10,f);
	fclose(f);

	/* open file and mmap it */
	fd = open(path,O_RDWR);
	if(fd < 0) {
		test_assertFalse(true);
		return;
	}
	addr = mmap(NULL,9000,9000,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	if(!addr) {
		test_assertFalse(true);
		return;
	}
	/* check content */
	for(i = 0; i < 9000; ++i) {
		char c = ((char*)addr)[i];
		test_assertInt(c,'0' + i % 10);
	}
	/* write new content */
	for(i = 0; i < 9000; ++i)
		((char*)addr)[i] = '0' + (10 - (i % 10));
	munmap(addr);

	/* check new content */
	f = fopen(path,"r");
	if(!f) {
		test_assertFalse(true);
		return;
	}
	for(i = 0; i < 9000; ++i)
		test_assertInt(fgetc(f),'0' + (10 - (i % 10)));
	fclose(f);

	test_assertInt(unlink(path),0);

	test_caseSucceeded();
}
