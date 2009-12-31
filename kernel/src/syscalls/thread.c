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

#include <common.h>
#include <task/thread.h>
#include <task/proc.h>
#include <mem/kheap.h>
#include <syscalls/thread.h>
#include <syscalls.h>
#include <errors.h>

void sysc_gettid(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,t->tid);
}

void sysc_getThreadCount(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,sll_length(t->proc->threads));
}

void sysc_startThread(sIntrptStackFrame *stack) {
	u32 entryPoint = SYSC_ARG1(stack);
	char **args = (char**)SYSC_ARG2(stack);
	char *argBuffer = NULL;
	u32 argSize = 0;
	s32 res,argc = 0;

	/* build arguments */
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,true);
		if(argc < 0)
			SYSC_ERROR(stack,argc);
	}

	res = proc_startThread(entryPoint,argc,argBuffer,argSize);
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* free arguments when new thread returns */
	if(res == 0)
		kheap_free(argBuffer);
	SYSC_RET1(stack,res);
}
