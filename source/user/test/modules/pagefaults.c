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
#include <esc/arch.h>
#include <sys/mman.h>
#include <esc/conf.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <stdio.h>

#include "../modules.h"

#define REG_SIZE		(1024 * 1024 * 4)
#define THREAD_COUNT	4
#define TEST_COUNT		1000

static void *regAddr;

static int thread_entry(A_UNUSED void *arg) {
	size_t i;
	for(i = 0; i < REG_SIZE / PAGE_SIZE; ++i)
		*(((volatile char*)regAddr) + i * PAGE_SIZE) = i;
	return 0;
}

static void dotest(int fd) {
	regAddr = mmap(NULL,REG_SIZE,REG_SIZE,PROT_READ | PROT_WRITE,MAP_PRIVATE,fd,0);
	if(regAddr == NULL)
		error("mmap failed");

	int i;
	for(i = 0; i < THREAD_COUNT; ++i) {
		if(startthread(thread_entry,NULL) < 0)
			error("startthread failed");
	}

	join(0);

	munmap(regAddr);
}

static int createfile(void) {
	char *buffer = calloc(1,PAGE_SIZE);
	if(!buffer)
		error("malloc failed");
	int fd = create("/sys/test",O_RDWR,0600);
	if(fd < 0)
		error("Unable to create /sys/test");
	size_t i;
	for(i = 0; i < REG_SIZE / PAGE_SIZE; ++i) {
		if(write(fd,buffer,PAGE_SIZE) != (ssize_t)PAGE_SIZE)
			error("write failed");
	}
	free(buffer);
	return fd;
}

int mod_pagefaults(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int i,fd = createfile();
	for(i = 0; i < TEST_COUNT; ++i)
		dotest(fd);
	close(fd);
	if(unlink("/sys/test") != 0)
		error("unlink failed");
	return EXIT_SUCCESS;
}
