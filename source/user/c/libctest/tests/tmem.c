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

#include <esc/common.h>
#include <esc/test.h>
#include <esc/mem.h>
#include <esc/elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "tmem.h"

/* forward declarations */
static void test_mem(void);
static void test_mmap_file(void);

/* our test-module */
sTestModule tModMem = {
	"Memory",
	&test_mem
};

static void test_mem(void) {
	test_mmap_file();
}

static void test_mmap_file(void) {
	void *addr;
	sElfEHeader *header;
	sFileInfo info;

	test_caseStart("Testing regadd with a file");

	int fd = open("/bin/libctest",IO_READ);
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
