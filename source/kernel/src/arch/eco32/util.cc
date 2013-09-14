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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/console.h>
#include <sys/dbg/kb.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/interrupts.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <stdarg.h>
#include <string.h>

static Util::FuncCall frames[1] = {
	{0,0,""}
};

void Util::panicArch() {
}

void Util::printUserStateOf(OStream &os,const Thread *t) {
	uintptr_t kstackAddr = DIR_MAPPED_SPACE | (t->getKernelStack() << PAGE_SIZE_SHIFT);
	uintptr_t istackAddr = (uintptr_t)t->getIntrptStack();
	if(istackAddr) {
		IntrptStackFrame *istack = (IntrptStackFrame*)(kstackAddr + (istackAddr & (PAGE_SIZE - 1)));
		os.writef("User state:\n");
		os.writef("\tPSW: 0x%08x\n\t",istack->psw);
		for(size_t i = 0; i < REG_COUNT; i++) {
			int row = i / 4;
			int col = i % 4;
			os.writef("$%-2d: 0x%08x ",col * 8 + row,istack->r[col * 8 + row]);
			if(i % 4 == 3)
				os.writef("\n\t");
		}
		os.writef("\n");
	}
}

void Util::printUserState(OStream &os) {
	const Thread *t = Thread::getRunning();
	printUserStateOf(os,t);
}

Util::FuncCall *Util::getUserStackTrace() {
	/* eco32 has no frame-pointer; therefore without information about the stackframe-sizes or
	 * similar, there is no way to determine the stacktrace */
	return frames;
}

Util::FuncCall *Util::getKernelStackTrace() {
	return frames;
}

Util::FuncCall *Util::getUserStackTraceOf(A_UNUSED Thread *t) {
	return frames;
}

Util::FuncCall *Util::getKernelStackTraceOf(A_UNUSED const Thread *t) {
	return frames;
}
