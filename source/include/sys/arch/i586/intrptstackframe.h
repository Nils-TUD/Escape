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

#if !defined(__cplusplus)
#	define SAVE_REGS		pusha
#	define RESTORE_REGS		popa
#	define IRET				iret
#else
#	include <sys/common.h>
#	include <sys/ostream.h>
#	include <assert.h>

class VM86;

/* the stack frame for the interrupt-handler */
class IntrptStackFrame {
	friend class VM86;

public:
	ulong getError() const {
		assert(intrptNo != 0);
		return errorCode;
	}

	// we don't need eflags on sysenter because we can expect that the user doesn't rely on a
	// not-changed eflags after the syscall. obviously, we need it on interrupts and thus we have
	// to save/restore it when handling a signal.
	// note that it has to be 1 << 9 (IF set) because if we handle the signal during a syscall and
	// restore it with int/iret we have to have a valid eflags value. So, at least IF has to be set.
	ulong getFlags() const {
		return intrptNo ? eflags : (1 << 9);
	}
	void setFlags(ulong flags) {
		if(intrptNo)
			eflags = flags;
	}

	ulong getIP() const {
		return intrptNo ? eip : edx;
	}
	void setIP(ulong ip) {
		if(intrptNo)
			eip = ip;
		else
			edx = ip;
	}

	ulong getSP() const {
		return intrptNo ? uesp : ecx;
	}
	void setSP(ulong sp) {
		if(intrptNo)
			uesp = sp;
		else
			ecx = sp;
	}

	void print(OStream &os) const {
		os.writef("stack-frame @ %p\n",this);
		os.writef("\teax: %#08x\n",eax);
		os.writef("\tebx: %#08x\n",ebx);
		os.writef("\tecx: %#08x\n",ecx);
		os.writef("\tedx: %#08x\n",edx);
		os.writef("\tesi: %#08x\n",esi);
		os.writef("\tedi: %#08x\n",edi);
		os.writef("\tebp: %#08x\n",ebp);
		os.writef("\tusp: %#08x\n",getSP());
		os.writef("\teip: %#08x\n",getIP());
		os.writef("\tefl: %#08x\n",getFlags());
		if(intrptNo) {
			os.writef("\terr: %d\n",getError());
			os.writef("\tint: %d\n",intrptNo);
		}
	}

	/* general purpose registers */
	ulong edi;
	ulong esi;
	ulong ebp;
	ulong : 32; /* esp from pusha */
	ulong ebx;
	ulong edx;
	ulong ecx;
	ulong eax;
	/* interrupt-number */
	ulong intrptNo;
private:
	/* error-code (for exceptions); default = 0 */
	ulong errorCode;
	/* pushed by the CPU */
	ulong eip;
	ulong cs;
	ulong eflags;
	/* if we come from user-mode this fields will be present and will be restored with iret */
	ulong uesp;
	ulong uss;
} A_PACKED;

struct VM86IntrptStackFrame : public IntrptStackFrame {
	ushort vm86es;
	ushort : 16;
	ushort vm86ds;
	ushort : 16;
	ushort vm86fs;
	ushort : 16;
	ushort vm86gs;
	ushort : 16;
} A_PACKED;

class Thread;
typedef void (*irqhandler_func)(Thread *t,IntrptStackFrame *stack);

#endif
