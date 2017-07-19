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

#include <dbg/console.h>
#include <dbg/kb.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <task/thread.h>
#include <common.h>
#include <interrupts.h>
#include <ksymbols.h>
#include <stdarg.h>
#include <string.h>
#include <util.h>
#include <video.h>

static uintptr_t frames[1] = {
	0
};

void Util::panicArch() {
}

void Util::printUserStateOf(OStream &os,const Thread *t) {
	uintptr_t kstackAddr = DIR_MAP_AREA | (t->getKernelStack() << PAGE_BITS);
	uintptr_t istackAddr = (uintptr_t)t->getUserState();
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

uintptr_t *Util::getUserStackTrace() {
	/* eco32 has no frame-pointer; therefore without information about the stackframe-sizes or
	 * similar, there is no way to determine the stacktrace */
	return frames;
}

uintptr_t *Util::getKernelStackTrace() {
	return frames;
}

uintptr_t *Util::getUserStackTraceOf(A_UNUSED Thread *t) {
	return frames;
}

uintptr_t *Util::getKernelStackTraceOf(A_UNUSED const Thread *t) {
	return frames;
}
