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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/smp.h>
#include <sys/mem/physmem.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/cpu.h>
#include <sys/interrupts.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <ipc/ipcbuf.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

void Util::printUserStateOf(OStream &os,const Thread *t) {
	if(t->getIntrptStack()) {
		frameno_t frame = t->getProc()->getPageDir()->getFrameNo(t->getKernelStack());
		uintptr_t kstackAddr = PageDir::getAccess(frame);
		size_t kstackOff = (uintptr_t)t->getIntrptStack() & (PAGE_SIZE - 1);
		IntrptStackFrame *kstack = (IntrptStackFrame*)(kstackAddr + kstackOff);
		os.writef("User-Register:\n");
		os.writef("\teax=%#08x, ebx=%#08x, ecx=%#08x, edx=%#08x\n",
				kstack->eax,kstack->ebx,kstack->ecx,kstack->edx);
		os.writef("\tesi=%#08x, edi=%#08x, esp=%#08x, ebp=%#08x\n",
				kstack->esi,kstack->edi,kstack->getSP(),kstack->ebp);
		os.writef("\teip=%#08x, eflags=%#08x\n",kstack->getIP(),kstack->getFlags());
		os.writef("\tcr0=%#08x, cr2=%#08x, cr3=%#08x, cr4=%#08x\n",
				CPU::getCR0(),CPU::getCR2(),CPU::getCR3(),CPU::getCR4());
		PageDir::removeAccess(frame);
	}
}

void Util::printUserState(OStream &os) {
	const Thread *t = Thread::getRunning();
	printUserStateOf(os,t);
}
