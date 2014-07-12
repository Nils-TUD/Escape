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

#pragma once

#include <common.h>
#include <interrupts.h>
#include <task/elf.h>
#include <task/thread.h>

class UEnvBase {
	UEnvBase() = delete;

public:
	/**
	 * Checks whether a signal should be handled. If so, it will be stored for later finishing and a
	 * thread-switch is done, if necessary.
	 *
	 * @param t the current thread
	 * @param stack the interrupt-stack-frame
	 */
	static void handleSignal(Thread *t,IntrptStackFrame *stack);

	/**
	 * Finishes the signal-handling-process
	 *
	 * @param stack the interrupt-stack-frame
	 * @return 0 on success
	 */
	static int finishSignalHandler(IntrptStackFrame *stack);

	/**
	 * Setups the user-stack for given interrupt-stack, when starting the current process
	 *
	 * @param argc the argument-count
	 * @param envc the env-count
	 * @param args the arguments+env one after another, allocated on the heap; may be NULL
	 * @param argsSize the total number of bytes for the arguments+env (just the data)
	 * @param info startup-info
	 * @param entryPoint the entry-point
	 * @param fd the file-descriptor for the executable for the dynamic linker (-1 if not needed)
	 * @return true if successfull
	 */
	static bool setupProc(int argc,int envc,const char *args,size_t argsSize,
		const ELF::StartupInfo *info,uintptr_t entryPoint,int fd);

	/**
	 * Setups the user-environment when starting the current thread
	 *
	 * @param arg the thread-argument
	 * @param tentryPoint the entry-point
	 * @return the stack-pointer
	 */
	static void *setupThread(const void *arg,uintptr_t tentryPoint) asm("uenv_setupThread");

protected:
	static ulong *initProcStack(int argc,int envc,const char *args,size_t argsSize);
	static ulong *initThreadStack(const void *arg,uintptr_t entry);
	static char **copyArgs(int argc,const char *&args,ulong *&sp);
	static ulong *addArgs(ulong *sp,uintptr_t tentryPoint,bool newThread);
};

#if defined(__x86__)
#	include <arch/x86/task/uenv.h>
#elif defined(__eco32__)
#	include <arch/eco32/task/uenv.h>
#elif defined(__mmix__)
#	include <arch/mmix/task/uenv.h>
#endif
