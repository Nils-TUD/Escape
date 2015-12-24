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
#	include <arch/x86_64/mem/layout.h>
#	define SAVE_REGS		push	%rax;	\
 							push	%rbx;	\
 							push	%rcx;	\
 							push	%rdx;	\
 							push	%rdi;	\
 							push	%rsi;	\
 							push	%rbp;	\
 							push	%r8;	\
 							push	%r9;	\
 							push	%r10;	\
 							push	%r11;	\
 							push	%r12;	\
 							push	%r13;	\
 							push	%r14;	\
 							push	%r15
#	define RESTORE_REGS		pop		%r15;	\
 							pop		%r14;	\
 							pop		%r13;	\
 							pop		%r12;	\
 							pop		%r11;	\
 							pop		%r10;	\
 							pop		%r9;	\
 							pop		%r8;	\
 							pop		%rbp;	\
 							pop		%rsi;	\
 							pop		%rdi;	\
 							pop		%rdx;	\
 							pop		%rcx;	\
 							pop		%rbx;	\
 							pop		%rax
#	define IRET				iretq

// the idea here is to read the stackpointer from the IA32_SYSENTER_ESP MSR. to do that, we need to
// put $0x175 into %rcx. rdmsr will put the result in %rax:%rdx. thus, we save %rsp,%rcx and %rdx
// first (%rax is not needed). afterwards, we build the stackpointer from %rax:%rdx and restore the
// registers
#	define SYS_ENTER		mov     %rsp,%r11;		\
 							mov		%rcx,%r9;		\
 							mov		%rdx,%rsp;		\
 							mov		$0x175,%rcx;	\
 							rdmsr;					\
 							shl     $32,%rdx;		\
 							mov		%rdx,%rcx;		\
 							mov		%rsp,%rdx;		\
 							mov		%rax,%rsp;		\
 							or 		%rcx,%rsp;		\
 							mov     %r11,%rax;		\
 							mov		%r9,%rcx;		\
							sub		$(7 * 8),%rsp;	// stack layout as for interrupts/exceptions
#	define SYS_LEAVE		mov		%r11,%rsp;		\
 							mov		$0x200,%r11;	\
 							sysretq
#else
#	include <common.h>
#	include <ostream.h>
#	include <assert.h>

/* the stack frame for the interrupt-handler */
class IntrptStackFrame {
public:
	bool fromUserSpace() const {
		// syscall or IF flag set
		return intrptNo == 0 || (rflags & (1 << 9));
	}

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
		return intrptNo ? rflags : (1 << 9);
	}
	void setFlags(ulong flags) {
		if(intrptNo)
			rflags = flags;
	}

	ulong getIP() const {
		return intrptNo ? rip : rcx;
	}
	void setIP(ulong ip) {
		if(intrptNo)
			rip = ip;
		else
			rcx = ip;
	}

	ulong getBP() const {
		return rbp;
	}
	ulong getSP() const {
		return intrptNo ? ursp : r11;
	}
	void setSP(ulong sp) {
		if(intrptNo)
			ursp = sp;
		else
			r11 = sp;
	}

	void setSS() {
		if((cs & 3) == 0)
			uss = (SEG_KDATA << 3) | 0;
	}

	void print(OStream &os) const {
		os.writef("\trax: %#016lx rbx: %#016lx\n",rax,rbx);
		os.writef("\trcx: %#016lx rdx: %#016lx\n",rcx,rdx);
		os.writef("\trsi: %#016lx rdi: %#016lx\n",rsi,rdi);
		os.writef("\trsp: %#016lx rbp: %#016lx\n",getSP(),getBP());
		os.writef("\tr8 : %#016lx r9 : %#016lx\n",r8,r9);
		os.writef("\tr10: %#016lx r11: %#016lx\n",r10,r11);
		os.writef("\tr12: %#016lx r13: %#016lx\n",r12,r13);
		os.writef("\tr14: %#016lx r15: %#016lx\n",r14,r15);
		os.writef("\trip: %#016lx rfl: %#016lx\n",getIP(),getFlags());
		if(intrptNo)
			os.writef("\terr: %10ld int: %10ld\n",errorCode,intrptNo);
	}

	/* general purpose registers */
	ulong r15;
	ulong r14;
	ulong r13;
	ulong r12;
	ulong r11;
	ulong r10;
	ulong r9;
	ulong r8;
	ulong rbp;
	ulong rsi;
	ulong rdi;
	ulong rdx;
	ulong rcx;
	ulong rbx;
	ulong rax;
	/* interrupt-number */
	ulong intrptNo;
private:
	/* error-code (for exceptions); default = 0 */
	ulong errorCode;
	/* pushed by the CPU */
	ulong rip;
	ulong cs;
	ulong rflags;
	/* if we come from user-mode this fields will be present and will be restored with iret */
	ulong ursp;
	ulong uss;
	/* on x86_64, interrupt stacks are 64-bit aligned */
	ulong : 64;
} A_PACKED;

class Thread;
typedef void (*irqhandler_func)(Thread *t,IntrptStackFrame *stack);

#endif
