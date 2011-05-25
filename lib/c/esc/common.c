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
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/dir.h>
#include <esc/register.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX_STACK_PAGES		128
#define PAGE_SIZE			4096
#define PROCINFO_BUF_SIZE	256
#define MAX_STACK_DEPTH		20
/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE		5

static char *getProcName(void);

int errno = 0;

/**
 * Displays an error-message according to given format and arguments and appends ': <errmsg>' if
 * errno is < 0. After that exit(EXIT_FAILURE) is called.
 *
 * @param fmt the error-message-format
 */
void error(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vprinte(fmt,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

uintptr_t *getStackTrace(void) {
	static uintptr_t frames[MAX_STACK_DEPTH];
#ifndef __eco32__
	uintptr_t end,start;
	size_t i;
	uint32_t *ebp;
	uintptr_t *frame = &frames[0];
	GET_REG("ebp",ebp);
	/* TODO just temporary */
	end = ((uintptr_t)ebp + (MAX_STACK_PAGES * PAGE_SIZE - 1)) & ~(MAX_STACK_PAGES * PAGE_SIZE - 1);
	start = end - PAGE_SIZE * MAX_STACK_PAGES;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((uintptr_t)ebp < start || (uintptr_t)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		ebp = (uint32_t*)*ebp;
		frame++;
	}
	/* terminate */
	*frame = 0;
#endif
	return &frames[0];
}

void printStackTrace(void) {
	uintptr_t *trace = getStackTrace();
	char *name = getProcName();
	debugf("Process %s - stack-trace:\n",name ? name : "???");
	/* TODO maybe we should skip printStackTrace here? */
	while(*trace != 0) {
		debugf("\t0x%08x\n",*trace);
		trace++;
	}
}

static char *getProcName(void) {
	static char name[MAX_NAME_LEN];
	char buffer[PROCINFO_BUF_SIZE];
	char path[MAX_PATH_LEN];
	tFD fd;
	snprintf(path,sizeof(path),"/system/processes/%d/info",getpid());
	fd = open(path,IO_READ);
	if(fd >= 0) {
		if(RETRY(read(fd,buffer,PROCINFO_BUF_SIZE - 1)) < 0) {
			close(fd);
			return NULL;
		}
		buffer[PROCINFO_BUF_SIZE - 1] = '\0';
		sscanf(
			buffer,
			"%*s%*hu" "%*s%*hu" "%*s%s",
			name
		);
		close(fd);
		return name;
	}
	return NULL;
}
