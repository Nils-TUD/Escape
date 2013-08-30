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
#include <sys/arch/i586/task/vm86.h>
#include <sys/arch/i586/task/ioports.h>
#include <sys/arch/i586/ports.h>
#include <sys/arch/i586/gdt.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/virtmem.h>
#include <sys/video.h>
#include <sys/mutex.h>
#include <sys/util.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define X86OP_INT			0xCD
#define X86OP_IRET			0xCF
#define X86OP_PUSHF			0x9C
#define X86OP_POPF			0x9D
#define X86OP_OUTW			0xEF
#define X86OP_OUTB			0xEE
#define X86OP_INW			0xED
#define X86OP_INB			0xEC
#define X86OP_STI			0xFA
#define X86OP_CLI			0xFB

#define X86OP_DATA32		0x66
#define X86OP_ADDR32		0x67
#define X86OP_CS			0x2E
#define X86OP_DS			0x3E
#define X86OP_ES			0x26
#define X86OP_SS			0x36
#define X86OP_GS			0x65
#define X86OP_FS			0x64
#define X86OP_REPNZ			0xF2
#define X86OP_REP			0xF3

#define VM86_IVT_SIZE		256
#define VM86_MAX_MEMPAGES	2

#define DBGVM86(fmt...)		/*Video::printf(fmt)*/

frameno_t VM86::frameNos[(1024 * K) / PAGE_SIZE];
tid_t VM86::vm86Tid = INVALID_TID;
volatile tid_t VM86::caller = INVALID_TID;
VM86::Info VM86::info;
int VM86::vm86Res = -1;
Mutex VM86::vm86Lock;

int VM86::create() {
	/* already present? */
	if(vm86Tid != INVALID_TID)
		return -EINVAL;

	/* create child */
	/* Note that it is really necessary to set whether we're a VM86-task or not BEFORE we get
	 * chosen by the scheduler the first time. Otherwise the scheduler can't set the right
	 * value for tss.esp0 and we will get a wrong stack-layout on the next interrupt */
	int res = Proc::clone(P_VM86);
	if(res < 0)
		return res;
	/* parent */
	if(res != 0)
		return 0;

	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	vm86Tid = t->getTid();

	/* remove all regions */
	Proc::removeRegions(p->getPid(),true);

	/* Now map the first MiB of physical memory to 0x00000000 and the first 64 KiB to 0x00100000,
	 * too. Because in real-mode it occurs an address-wraparound at 1 MiB. In VM86-mode it doesn't
	 * therefore we have to emulate it. We do that by simply mapping the same to >= 1MiB. */
	size_t frameCount = (1024 * K) / PAGE_SIZE;
	for(size_t i = 0; i < frameCount; i++)
		frameNos[i] = i;
	PageDir::mapToCur(0x00000000,frameNos,frameCount,PG_PRESENT | PG_WRITABLE);
	PageDir::mapToCur(0x00100000,frameNos,(64 * K) / PAGE_SIZE,PG_PRESENT | PG_WRITABLE);

	/* Give the vm86-task permission for all ports. As it seems vmware expects that if they
	 * have used the 32-bit-data-prefix once (at least for inw) it takes effect for the
	 * following instructions, too!? By giving the task the permission to perform port I/O
	 * directly we prevent this problem :) */
	/* FIXME but there has to be a better way.. */
	if(p->ioMap == NULL)
		p->ioMap = (uint8_t*)Cache::alloc(GDT::IO_MAP_SIZE / 8);
	/* note that we HAVE TO request all ports (even the reserved ones); otherwise it doesn't work
	 * everywhere (e.g. my notebook needs it) */
	if(p->ioMap != NULL)
		memset(p->ioMap,0x00,GDT::IO_MAP_SIZE / 8);

	/* give it a name */
	p->setCommand("VM86",0,"");

	/* block us; we get waked up as soon as someone wants to use us */
	Event::block(t);
	Thread::switchAway();

	/* ok, we're back again... */
	start();
	/* never reached */
	return 0;
}

int VM86::interrupt(uint16_t interrupt,USER Regs *regs,USER const Memarea *area) {
	if(area && BYTES_2_PAGES(area->size) > VM86_MAX_MEMPAGES)
		return -EINVAL;
	if(interrupt >= VM86_IVT_SIZE)
		return -EINVAL;
	Thread *t = Thread::getRunning();

	/* check whether there still is a vm86-task */
	Thread *vm86t = Thread::getById(vm86Tid);
	if(vm86t == NULL || !(vm86t->getProc()->getFlags() & P_VM86))
		return -ESRCH;

	/* ensure that only one thread at a time can use the vm86-task */
	vm86Lock.down();
	/* store information in calling process */
	caller = t->getTid();
	if(!copyInfo(interrupt,regs,area)) {
		finish();
		return -ENOMEM;
	}

	/* reserve frames for vm86-thread */
	if(info.area) {
		if(!vm86t->reserveFrames(BYTES_2_PAGES(info.area->size))) {
			finish();
			return -ENOMEM;
		}
	}

	/* make vm86 ready */
	Event::unblock(vm86t);

	/* block the calling thread and then do a switch */
	/* we'll wakeup the thread as soon as the vm86-task is done with the interrupt */
	Event::block(t);
	Thread::switchAway();

	/* everything is finished :) */
	if(vm86Res == 0) {
		Thread::addCallback(finish);
		memcpy(regs,&info.regs,sizeof(Regs));
		copyAreaResult();
		Thread::remCallback(finish);
	}

	/* mark as done */
	finish();
	return vm86Res;
}

void VM86::handleGPF(VM86IntrptStackFrame *stack) {
	uint8_t *ops = (uint8_t*)(stack->eip + (stack->cs << 4));
	uint8_t opCode;
	bool data32 = false;
	bool pref_done = false;

	do {
		switch((opCode = *ops)) {
			case X86OP_DATA32:
				data32 = true;
				break;
			case X86OP_ADDR32: break;
			case X86OP_CS: break;
			case X86OP_DS: break;
			case X86OP_ES: break;
			case X86OP_SS: break;
			case X86OP_GS: break;
			case X86OP_FS: break;
			case X86OP_REPNZ: break;
			case X86OP_REP: break;
			default:
				pref_done = true;
				break;
		}
		ops++;
	}
	while(!pref_done);

	switch(opCode) {
		case X86OP_INT: {
			uint16_t *sp;
			volatile uint32_t *ivt; /* has to be volatile to prevent llvm from optimizing it away */
			uint32_t intno = *ops;
			stack->uesp -= sizeof(uint16_t) * 3;
			sp = (uint16_t*)(stack->uesp + (stack->uss << 4));
			/* save eflags and ip on stack */
			sp[2] = (uint16_t)stack->eflags;
			sp[1] = (uint16_t)stack->cs;
			sp[0] = (uint16_t)stack->eip + 2;
			/* set new ip */
			ivt = (uint32_t*)0;
			assert(intno < VM86_IVT_SIZE);
			DBGVM86("[VM86] int 0x%x @ %x:%x -> %x:%x\n",intno,stack->cs,stack->eip + 2,
					ivt[intno] >> 16,ivt[intno] & 0xFFFF);
			stack->eip = ivt[intno] & 0xFFFF;
			stack->cs = ivt[intno] >> 16;
		}
		break;
		case X86OP_IRET: {
			uint32_t newip,newcs,newflags;
			if(data32) {
				newip = popl(stack);
				newcs = popl(stack) & 0xFFFF;
				newflags = popl(stack);
			}
			else {
				newip = popw(stack);
				newcs = popw(stack);
				newflags = (stack->eflags & 0xFFFF0000) | popw(stack);
			}
			DBGVM86("[VM86] iret -> (%x:%x,0x%x)\n",newcs,newip,newflags);
			/* eip = cs = 0 means we're done */
			if(newip == 0 && newcs == 0) {
				DBGVM86("[VM86] done\n");
				stop(stack);
				/* don't continue here */
				return;
			}
            stack->eip = newip;
            stack->cs = newcs;
            /* don't disable interrupts */
            stack->eflags = newflags | (1 << 9);
		}
		break;
		case X86OP_PUSHF:
			/* save eflags */
			DBGVM86("[VM86] pushf (0x%x)\n",stack->eflags);
			if(data32)
				pushl(stack,stack->eflags);
			else
				pushw(stack,(uint16_t)stack->eflags);
			stack->eip++;
			break;
		case X86OP_POPF:
			/* restore eflags (don't disable interrupts) */
			if(data32)
				stack->eflags = popl(stack) | (1 << 9);
			else
				stack->eflags = (stack->eflags & 0xFFFF0000) | popw(stack) | (1 << 9);
			DBGVM86("[VM86] popf (0x%x)\n",stack->eflags);
			stack->eip++;
			break;
		case X86OP_OUTW:
			DBGVM86("[VM86] outw (0x%x -> 0x%x)\n",stack->eax,stack->edx);
			if(data32)
				Ports::out<uint32_t>(stack->edx,stack->eax);
			else
				Ports::out<uint16_t>(stack->edx,stack->eax);
			stack->eip++;
			break;
		case X86OP_OUTB:
			DBGVM86("[VM86] outb (0x%x -> 0x%x)\n",stack->eax,stack->edx);
			Ports::out<uint8_t>(stack->edx,stack->eax);
			stack->eip++;
			break;
		case X86OP_INW:
			if(data32)
				stack->eax = Ports::in<uint32_t>(stack->edx);
			else
				stack->eax = Ports::in<uint16_t>(stack->edx);
			DBGVM86("[VM86] inw (0x%x <- 0x%x)\n",stack->eax,stack->edx);
			stack->eip++;
			break;
		case X86OP_INB:
			stack->eax = (stack->eax & 0xFF00) + Ports::in<uint8_t>(stack->edx);
			DBGVM86("[VM86] inb (0x%x <- 0x%x)\n",stack->eax,stack->edx);
			stack->eip++;
			break;
		case X86OP_STI:
			stack->eip++;
			break;
		case X86OP_CLI:
			stack->eip++;
			break;
		default:
			Util::panic("Invalid opcode (0x%x) @ 0x%x",opCode,(uintptr_t)(ops - 1));
			break;
	}
}

uint16_t VM86::popw(VM86IntrptStackFrame *stack) {
	uint16_t *sp = (uint16_t*)(stack->uesp + (stack->uss << 4));
	stack->uesp += sizeof(uint16_t);
	return *sp;
}

uint32_t VM86::popl(VM86IntrptStackFrame *stack) {
	uint32_t *sp = (uint32_t*)(stack->uesp + (stack->uss << 4));
	stack->uesp += sizeof(uint32_t);
	return *sp;
}

void VM86::pushw(VM86IntrptStackFrame *stack,uint16_t word) {
	stack->uesp -= sizeof(uint16_t);
	*((uint16_t*)(stack->uesp + (stack->uss << 4))) = word;
}

void VM86::pushl(VM86IntrptStackFrame *stack,uint32_t l) {
	stack->uesp -= sizeof(uint32_t);
	*((uint32_t*)(stack->uesp + (stack->uss << 4))) = l;
}

void VM86::start() {
	Thread *t = Thread::getRunning();
	assert(caller != INVALID_TID);

	/* copy the area to vm86; important: don't let the bios overwrite itself. therefore
	 * we map other frames to that area. */
	if(info.area) {
		/* can't fail */
		assert(PageDir::mapToCur(info.area->dst,NULL,BYTES_2_PAGES(info.area->size),
				PG_PRESENT | PG_WRITABLE) >= 0);
		memcpy((void*)info.area->dst,info.copies[0],info.area->size);
	}

	/* has to be volatile, because otherwise the compiler thinks we don't use it */
	volatile VM86IntrptStackFrame istack;
	/* set stack-pointer (in an unsed area) */
	/* the BIOS puts the bootloader to 0x7C00. it is 512 bytes large, so that the area
	 * 0x7C00 .. 0x7E00 should always be available (we don't need it anymore). */
	istack.uss = 0x700;
	istack.uesp = 0xDF8;
	/* ensure that the last 2 words on the stack are 0. it is required for the last iret that
	 * indicates that vm86 is finished */
	memclear((void*)0x7DF8,0x8);
	/* enable VM and IF flag, IOPL = 0*/
	istack.eflags = (1 << 17) | (1 << 9);
	/* has to be volatile to prevent llvm from optimizing it away */
	volatile uint32_t *ivt = (uint32_t*)0;
	/* set entrypoint */
	istack.eip = ivt[info.interrupt] & 0xFFFF;
	istack.cs = ivt[info.interrupt] >> 16;
	/* segment registers */
	istack.vm86ds = info.regs.ds;
	istack.vm86es = info.regs.es;
	istack.vm86fs = 0;
	istack.vm86gs = 0;
	/* general purpose registers */
	istack.eax = info.regs.ax;
	istack.ebx = info.regs.bx;
	istack.ecx = info.regs.cx;
	istack.edx = info.regs.dx;
	istack.esi = info.regs.si;
	istack.edi = info.regs.di;
	istack.ebp = 0;
	t->popIntrptLevel();
	/* return to userland. do it here, so that we don't need the same stacklayout for VM86-mode and
	 * protected mode */
	asm volatile (
		"mov	%0,%%esp\n"
		"popa\n"
		"add	$8,%%esp\n"
		"iret"
		: : "r"(&istack)
	);
	A_UNREACHED;
}

void VM86::stop(VM86IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	Thread *ct = Thread::getById(caller);
	vm86Res = 0;
	if(ct != NULL) {
		copyRegResult(stack);
		vm86Res = storeAreaResult();
		Event::unblock(ct);
	}

	/* block us and do a switch */
	Event::block(t);
	Thread::switchAway();

	/* lets start with a new request :) */
	start();
}

void VM86::finish() {
	if(info.area)
		Thread::getById(vm86Tid)->discardFrames();
	clearInfo();
	caller = INVALID_TID;
	vm86Lock.up();
}

void VM86::copyRegResult(VM86IntrptStackFrame *stack) {
	info.regs.ax = stack->eax;
	info.regs.bx = stack->ebx;
	info.regs.cx = stack->ecx;
	info.regs.dx = stack->edx;
	info.regs.si = stack->esi;
	info.regs.di = stack->edi;
	info.regs.ds = stack->vm86ds;
	info.regs.es = stack->vm86es;
}

int VM86::storeAreaResult() {
	if(info.area) {
		uintptr_t start = info.area->dst / PAGE_SIZE;
		size_t pages = BYTES_2_PAGES(info.area->size);
		/* copy the result to heap; we'll copy it later to the calling process */
		memcpy(info.copies[0],(void*)info.area->dst,info.area->size);
		for(size_t i = 0; i < info.area->ptrCount; i++) {
			uintptr_t rmAddr = *(uintptr_t*)((uintptr_t)info.copies[0] + info.area->ptr[i].offset);
			uintptr_t virt = ((rmAddr & 0xFFFF0000) >> 12) | (rmAddr & 0xFFFF);
			memcpy((void*)info.copies[i + 1],(void*)virt,info.area->ptr[i].size);
		}
		/* undo mapping */
		PageDir::unmapFromCur(info.area->dst,pages,true);
		for(size_t i = 0; i < pages; i++)
			frameNos[start + i] = start + i;
		assert(PageDir::mapToCur(info.area->dst,frameNos + start,pages,PG_PRESENT | PG_WRITABLE) == 0);
	}
	return 0;
}

void VM86::copyAreaResult() {
	if(info.area) {
		memcpy(info.area->src,info.copies[0],info.area->size);
		if(info.area->ptrCount > 0) {
			for(size_t i = 0; i < info.area->ptrCount; i++)
				memcpy((void*)info.area->ptr[i].result,info.copies[i + 1],info.area->ptr[i].size);
		}
	}
}

bool VM86::copyInfo(uint16_t interrupt,USER const Regs *regs,USER const Memarea *area) {
	info.interrupt = interrupt;
	memcpy(&info.regs,regs,sizeof(Regs));
	info.copies = NULL;
	info.area = NULL;
	if(area) {
		/* copy area */
		info.area = (Memarea*)Cache::alloc(sizeof(Memarea));
		if(info.area == NULL)
			return false;
		memcpy(info.area,area,sizeof(Memarea));
		/* copy ptrs */
		info.area->ptr = NULL;
		if(info.area->ptrCount > 0) {
			info.area->ptr = (AreaPtr*)Cache::alloc(sizeof(AreaPtr) * info.area->ptrCount);
			if(!info.area->ptr) {
				clearInfo();
				return false;
			}
			memcpy(info.area->ptr,area->ptr,sizeof(AreaPtr) * info.area->ptrCount);
		}
		/* create buffers for the data-exchange */
		info.copies = (void**)Cache::calloc(1 + area->ptrCount,sizeof(void*));
		info.copies[0] = Cache::alloc(info.area->size);
		if(!info.copies[0]) {
			clearInfo();
			return false;
		}
		memcpy(info.copies[0],area->src,area->size);
		for(size_t i = 0; i < area->ptrCount; i++) {
			void *copy = Cache::alloc(area->ptr[i].size);
			if(!copy) {
				clearInfo();
				return false;
			}
			info.copies[i + 1] = copy;
		}
	}
	return true;
}

void VM86::clearInfo() {
	if(info.area) {
		for(size_t i = 0; i <= info.area->ptrCount; i++)
			Cache::free(info.copies[i]);
		Cache::free(info.area->ptr);
		Cache::free(info.copies);
		Cache::free(info.area);
	}
}
