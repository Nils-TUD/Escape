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

#pragma once

#include <sys/common.h>
#include <sys/arch/i586/fpu.h>

#define STACK_REG_COUNT			1

/* the thread-state which will be saved for context-switching */
typedef struct {
	uint32_t esp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t eflags;
	uint32_t ebx;
	/* note that we don't need to save eip because when we're done in thread_resume() we have
	 * our kernel-stack back which causes the ret-instruction to return to the point where
	 * we've called thread_save(). the user-eip is saved on the kernel-stack anyway.. */
	/* note also that this only works because when we call thread_save() in Proc::finishClone
	 * we take care not to call a function afterwards (since it would overwrite the return-address
	 * on the stack). When we call it in thread_switch() our return-address gets overwritten, but
	 * it doesn't really matter because it looks like this:
	 * if(!thread_save(...)) {
	 * 		// old thread
	 * 		// call functions ...
	 * 		thread_resume(...);
	 * }
	 * So whether we return to the instruction after the call of thread_save and jump below this
	 * if-statement or whether we return to the instruction after thread_resume() doesn't matter.
	 */
} sThreadRegs;
