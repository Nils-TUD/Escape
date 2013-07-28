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
#include <sys/dbg/console.h>
#include <sys/dbg/kb.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/cache.h>
#include <sys/debug.h>
#include <sys/interrupts.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/video.h>
#include <stdarg.h>
#include <string.h>

static Util::FuncCall frames[1] = {
	{0,0,""}
};

void Util::panicArch() {
}

void Util::printUserStateOf(const Thread *t) {
	sKSpecRegs *sregs = t->getSpecRegs();
	vid_printf("User state:\n");
	prf_pushIndent();
	Interrupts::printStackFrame(t->getIntrptStack());
	vid_printf("rBB : #%016lx rWW : #%016lx rXX : #%016lx\n",sregs->rbb,sregs->rww,sregs->rxx);
	vid_printf("rYY : #%016lx rZZ : #%016lx\n",sregs->ryy,sregs->rzz);
	prf_popIndent();
}

void Util::printUserState() {
	const Thread *t = Thread::getRunning();
	Util::printUserStateOf(t);
}

Util::FuncCall *Util::getUserStackTrace() {
	/* TODO */
	/* the MMIX-toolchain doesn't use a frame-pointer when enabling optimization, as it seems.
	 * therefore without information about the stackframe-sizes or similar, there is no way to
	 * determine the stacktrace */
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
