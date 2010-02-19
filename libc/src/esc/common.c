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
#include <esc/fileio.h>
#include <register.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define PROCINFO_BUF_SIZE	256
#define MAX_STACK_DEPTH		20
/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE		5

static char *getProcName(void);

s32 errno = 0;

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

u32 *getStackTrace(void) {
	static u32 frames[MAX_STACK_DEPTH];
	u32 i;
	u32 *ebp;
	/* TODO just temporary */
	u32 end = 0xC0000000;
	u32 start = end - 0x1000 * 2;
	u32 *frame = &frames[0];

	GET_REG("ebp",ebp)
	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((u32)ebp < start || (u32)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		ebp = (u32*)*ebp;
		frame++;
	}

	/* terminate */
	*frame = 0;
	return &frames[0];
}

void printStackTrace(void) {
	u32 *trace = getStackTrace();
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
		if(read(fd,buffer,PROCINFO_BUF_SIZE - 1) < 0) {
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
