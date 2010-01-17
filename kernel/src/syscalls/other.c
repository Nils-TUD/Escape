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
#include <multiboot.h>
#include <machine/intrpt.h>
#include <mem/paging.h>
#include <task/thread.h>
#include <task/lock.h>
#include <syscalls/other.h>
#include <syscalls.h>
#include <video.h>

void sysc_loadMods(sIntrptStackFrame *stack) {
	UNUSED(stack);
	mboot_loadModules(intrpt_getCurStack());
}

void sysc_debugc(sIntrptStackFrame *stack) {
	vid_putchar((char)SYSC_ARG1(stack));
}

void sysc_debug(sIntrptStackFrame *stack) {
	UNUSED(stack);
	proc_dbg_print(proc_getRunning());
	/*vfsn_dbg_printTree();
	paging_dbg_printOwnPageDir(PD_PART_USER);*/
}

void sysc_lock(sIntrptStackFrame *stack) {
	u32 ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 res;

	res = lock_aquire(t->tid,global ? INVALID_PID : t->proc->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unlock(sIntrptStackFrame *stack) {
	u32 ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 res;

	res = lock_release(global ? INVALID_PID : t->proc->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
