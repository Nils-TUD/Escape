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
#include <mem/cache.h>
#include <mem/virtmem.h>
#include <common.h>
#include <cpu.h>
#include <interrupts.h>
#include <stdarg.h>
#include <string.h>
#include <util.h>
#include <video.h>

static Util::FuncCall frames[1] = {
	{0,0,""}
};

void Util::panicArch() {
}

void Util::printUserStateOf(OStream &os,const Thread *t) {
	KSpecRegs *sregs = t->getSpecRegs();
	os.writef("User state:\n");
	os.pushIndent();
	Interrupts::printStackFrame(os,t->getIntrptStack());
	os.writef("rBB : #%016lx rWW : #%016lx rXX : #%016lx\n",sregs->rbb,sregs->rww,sregs->rxx);
	os.writef("rYY : #%016lx rZZ : #%016lx\n",sregs->ryy,sregs->rzz);
	os.popIndent();
}

void Util::printUserState(OStream &os) {
	const Thread *t = Thread::getRunning();
	Util::printUserStateOf(os,t);
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
