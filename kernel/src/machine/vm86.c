/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <common.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/sched.h>
#include <task/signals.h>
#include <mem/kheap.h>
#include <mem/paging.h>
#include <mem/vmm.h>
#include <machine/vm86.h>
#include <machine/gdt.h>
#include <util.h>
#include <video.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

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

#define DBGVM86(fmt...)		/*vid_printf(fmt)*/

static u16 vm86_popw(sIntrptStackFrame *stack);
static u32 vm86_popl(sIntrptStackFrame *stack);
static void vm86_pushw(sIntrptStackFrame *stack,u16 word);
static void vm86_pushl(sIntrptStackFrame *stack,u32 l);
static void vm86_start(void);
static void vm86_stop(sIntrptStackFrame *stack);
static void vm86_copyRegResult(sIntrptStackFrame* stack);
static void vm86_copyPtrResult(sVM86Memarea *areas,u16 areaCount);
static sVM86Info *vm86_createInfo(u16 interrupt,sVM86Regs *regs,sVM86Memarea *areas,u16 areaCount);
static void vm86_destroyInfo(sVM86Info *i);

static u32 *frameNos = NULL;
static tTid vm86Tid = INVALID_TID;
static tTid caller = INVALID_TID;
static sVM86Info *info = NULL;
static s32 vm86Res = -1;

s32 vm86_create(void) {
	sProc *p;
	sThread *t;
	tPid pid;
	u32 i,frameCount;
	s32 res;

	pid = proc_getFreePid();
	if(pid == INVALID_PID)
		return ERR_NO_FREE_PROCS;

	frameNos = (u32*)kheap_alloc(((1024 * K) / PAGE_SIZE) * sizeof(u32));
	if(frameNos == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create child */
	/* Note that it is really necessary to set whether we're a VM86-task or not BEFORE we get
	 * chosen by the scheduler the first time. Otherwise the scheduler can't set the right
	 * value for tss.esp0 and we will get a wrong stack-layout on the next interrupt */
	res = proc_clone(pid,true);
	if(res < 0)
		return res;
	/* parent */
	if(res == 0)
		return 0;

	t = thread_getRunning();
	p = t->proc;
	vm86Tid = t->tid;

	/* remove all regions */
	vmm_removeAll(p,true);
	/* unset stack-region, so that we can't access it anymore */
	t->stackRegion = -1;

	/* Now map the first MiB of physical memory to 0x00000000 and the first 64 KiB to 0x00100000,
	 * too. Because in real-mode it occurs an address-wraparound at 1 MiB. In VM86-mode it doesn't
	 * therefore we have to emulate it. We do that by simply mapping the same to >= 1MiB. */
	frameCount = (1024 * K) / PAGE_SIZE;
	for(i = 0; i < frameCount; i++)
		frameNos[i] = i;
	paging_map(0x00000000,frameNos,frameCount,PG_PRESENT | PG_WRITABLE);
	paging_map(0x00100000,frameNos,(64 * K) / PAGE_SIZE,PG_PRESENT | PG_WRITABLE);

	/* Give the vm86-task permission for all ports. As it seems vmware expects that if they
	 * have used the 32-bit-data-prefix once (at least for inw) it takes effect for the
	 * following instructions, too!? By giving the task the permission to perform port I/O
	 * directly we prevent this problem :) */
	/* FIXME but there has to be a better way.. */
	if(p->ioMap == NULL)
		p->ioMap = (u8*)kheap_alloc(IO_MAP_SIZE / 8);
	if(p->ioMap != NULL)
		memset(p->ioMap,0x00,IO_MAP_SIZE / 8);

	/* give it a name */
	strcpy(p->command,"VM86");

	/* block us; we get waked up as soon as someone wants to use us */
	sched_setBlocked(t);
	thread_switchNoSigs();

	/* ok, we're back again... */
	vm86_start();
	/* never reached */
	return 0;
}

s32 vm86_int(u16 interrupt,sVM86Regs *regs,sVM86Memarea *areas,u16 areaCount) {
	u32 i;
	sThread *t;
	sThread *vm86t;
	volatile sVM86Info **volInfo;
	for(i = 0; i < areaCount; i++) {
		if((areas[i].type == VM86_MEM_DIRECT &&
				BYTES_2_PAGES(areas[i].data.direct.size) > VM86_MAX_MEMPAGES) ||
			(areas[i].type == VM86_MEM_PTR &&
				BYTES_2_PAGES(areas[i].data.ptr.size) > VM86_MAX_MEMPAGES)) {
			return ERR_INVALID_ARGS;
		}
	}
	if(interrupt >= VM86_IVT_SIZE)
		return ERR_INVALID_ARGS;
	t = thread_getRunning();

	/* check whether there still is a vm86-task */
	vm86t = thread_getById(vm86Tid);
	if(vm86t == NULL || !vm86t->proc->isVM86)
		return ERR_NO_VM86_TASK;

	/* if the vm86-task is active, wait here */
	volInfo = (volatile sVM86Info **)&info;
	while(*volInfo != NULL) {
		/* TODO we have a problem if the process that currently uses vm86 gets killed... */
		/* because we'll never get notified that we can use vm86 */
		thread_wait(t->tid,0,EV_VM86_READY);
		thread_switchNoSigs();
	}

	/* store information in calling process */
	caller = t->tid;
	info = vm86_createInfo(interrupt,regs,areas,areaCount);
	if(info == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* collect the frame-numbers for the mapping */
	for(i = 0; i < areaCount; i++) {
		if(areas[i].type == VM86_MEM_DIRECT) {
			/* TODO */
			/*paging_getFrameNos(info->mFrameNos + i * VM86_MAX_MEMPAGES,
					(u32)areas[i].data.direct.src,areas[i].data.direct.size);*/
		}
	}

	/* make vm86 ready */
	sched_setReady(vm86t);

	/* block the calling thread and then do a switch */
	/* we'll wakeup the thread as soon as the vm86-task is done with the interrupt */
	sched_setBlocked(thread_getRunning());
	thread_switch();

	/* everything is finished :) */
	if(vm86Res == 0) {
		memcpy(regs,&info->regs,sizeof(sVM86Regs));
		vm86_copyPtrResult(areas,areaCount);
	}

	/* mark as done */
	vm86_destroyInfo(info);
	info = NULL;
	caller = INVALID_TID;
	thread_wakeupAll(0,EV_VM86_READY);
	return vm86Res;
}

bool vm86_handleGPF(sIntrptStackFrame *stack) {
	u8 *ops = (u8*)(stack->eip + (stack->cs << 4));
	u8 opCode;
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
			u16 *sp;
			u32 *ivt,intno = *ops;
			stack->uesp -= sizeof(u16) * 3;
			sp = (u16*)(stack->uesp + (stack->uss << 4));
			/* save eflags and ip on stack */
			sp[0] = (u16)stack->eflags;
			sp[1] = (u16)stack->cs;
			sp[2] = (u16)stack->eip + 2;
			/* set new ip */
			ivt = (u32*)0;
			assert(intno < VM86_IVT_SIZE);
			DBGVM86("[VM86] int 0x%x @ %x:%x -> %x:%x\n",intno,stack->cs,stack->eip + 2,
					ivt[intno] >> 16,ivt[intno] & 0xFFFF);
			stack->eip = ivt[intno] & 0xFFFF;
			stack->cs = ivt[intno] >> 16;
		}
		break;
		case X86OP_IRET: {
			u32 newip,newcs,newflags;
			if(data32) {
				newflags = vm86_popl(stack);
				newcs = vm86_popl(stack);
				newip = vm86_popl(stack);
			}
			else {
				newflags = (stack->eflags & 0xFFFF0000) | vm86_popw(stack);
				newcs = vm86_popw(stack);
				newip = vm86_popw(stack);
			}
			DBGVM86("[VM86] iret -> (%x:%x,0x%x)\n",newcs,newip,newflags);
			/* eip = cs = 0 means we're done */
			if(newip == 0 && newcs == 0) {
				DBGVM86("[VM86] done\n");
				vm86_stop(stack);
				/* don't continue here */
				return true;
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
				vm86_pushl(stack,stack->eflags);
			else
				vm86_pushw(stack,(u16)stack->eflags);
			stack->eip++;
			break;
		case X86OP_POPF:
			/* restore eflags (don't disable interrupts) */
			if(data32)
				stack->eflags = vm86_popl(stack) | (1 << 9);
			else
				stack->eflags = (stack->eflags & 0xFFFF0000) | vm86_popw(stack) | (1 << 9);
			DBGVM86("[VM86] popf (0x%x)\n",stack->eflags);
			stack->eip++;
			break;
		case X86OP_OUTW:
			DBGVM86("[VM86] outw (0x%x -> 0x%x)\n",stack->eax,stack->edx);
			if(data32)
				util_outDWord(stack->edx,stack->eax);
			else
				util_outWord(stack->edx,stack->eax);
			stack->eip++;
			break;
		case X86OP_OUTB:
			DBGVM86("[VM86] outb (0x%x -> 0x%x)\n",stack->eax,stack->edx);
			util_outByte(stack->edx,stack->eax);
			stack->eip++;
			break;
		case X86OP_INW:
			if(data32)
				stack->eax = util_inDWord(stack->edx);
			else
				stack->eax = util_inWord(stack->edx);
			DBGVM86("[VM86] inw (0x%x <- 0x%x)\n",stack->eax,stack->edx);
			stack->eip++;
			break;
		case X86OP_INB:
			stack->eax = (stack->eax & 0xFF00) + util_inByte(stack->edx);
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
			util_panic("Invalid opcode (0x%x) @ 0x%x",opCode,(u32)(ops - 1));
			break;
	}
	return true;
}

static u16 vm86_popw(sIntrptStackFrame *stack) {
	u16 *sp = (u16*)(stack->uesp + (stack->uss << 4));
	stack->uesp += sizeof(u16);
	return *sp;
}

static u32 vm86_popl(sIntrptStackFrame *stack) {
	u32 *sp = (u32*)(stack->uesp + (stack->uss << 4));
	stack->uesp += sizeof(u32);
	return *sp;
}

static void vm86_pushw(sIntrptStackFrame *stack,u16 word) {
	stack->uesp -= sizeof(u16);
	*((u16*)(stack->uesp + (stack->uss << 4))) = word;
}

static void vm86_pushl(sIntrptStackFrame *stack,u32 l) {
	stack->uesp -= sizeof(u32);
	*((u32*)(stack->uesp + (stack->uss << 4))) = l;
}

static void vm86_start(void) {
	u32 *ivt;
	u32 i,j,frameCnt;
	sIntrptStackFrame *istack;
	assert(caller != INVALID_TID);
	assert(info != NULL);

start:
	istack = intrpt_getCurStack();

	/* undo the mappings of the previous call */
	for(i = 0; i < (1024 * K) / PAGE_SIZE; i++) {
		if(frameNos[i] != i) {
			paging_map(i * PAGE_SIZE,frameNos + i,1,PG_PRESENT | PG_WRITABLE);
			frameNos[i] = i;
		}
	}

	/* count the number of needed frames */
	frameCnt = 0;
	for(i = 0; i < info->areaCount; i++) {
		if(info->areas[i].type == VM86_MEM_DIRECT)
			frameCnt += BYTES_2_PAGES(info->areas[i].data.direct.size);
	}

	/* if there is not enough mem, stop here */
	/* Note that we can't determine that before because the amount of physmem may change in the
	 * meanwhile */
	if(mm_getFreeFrmCount(MM_DEF) < frameCnt) {
		vm86Res = ERR_NOT_ENOUGH_MEM;
		/* make caller ready, block us and do a switch */
		sched_setReady(thread_getById(caller));
		sched_setBlocked(thread_getRunning());
		thread_switchNoSigs();
		goto start;
	}

	/* map the specified areas to the frames of the parent, so that the BIOS can write
	 * it directly to the buffer of the calling process */
	for(i = 0; i < info->areaCount; i++) {
		if(info->areas[i].type == VM86_MEM_DIRECT) {
			u32 start = info->areas[i].data.direct.dst / PAGE_SIZE;
			u32 pages = BYTES_2_PAGES(info->areas[i].data.direct.size);
			for(j = 0; j < pages; j++)
				frameNos[start + j] = mm_allocateFrame(MM_DEF);
			paging_map(info->areas[i].data.direct.dst,frameNos + start,pages,PG_PRESENT | PG_WRITABLE);
		}
	}

	/* set stack-pointer (in an unsed area) */
	istack->uss = 0x9000;
	istack->uesp = 0x0FFC;
	/* enable VM flag */
	istack->eflags |= 1 << 17;
	/* set entrypoint */
	ivt = (u32*)0;
	istack->eip = ivt[info->interrupt] & 0xFFFF;
	istack->cs = ivt[info->interrupt] >> 16;
	/* segment registers */
	istack->vm86ds = info->regs.ds;
	istack->vm86es = info->regs.es;
	istack->vm86fs = 0;
	istack->vm86gs = 0;
	/* general purpose registers */
	istack->eax = info->regs.ax;
	istack->ebx = info->regs.bx;
	istack->ecx = info->regs.cx;
	istack->edx = info->regs.dx;
	istack->esi = info->regs.si;
	istack->edi = info->regs.di;
}

static void vm86_stop(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	sThread *ct = thread_getById(caller);
	if(ct != NULL) {
		vm86_copyRegResult(stack);
		sched_setReady(ct);
	}
	vm86Res = 0;

	/* block us and do a switch */
	sched_setBlocked(t);
	thread_switchNoSigs();

	/* lets start with a new request :) */
	vm86_start();
}

static void vm86_copyRegResult(sIntrptStackFrame *stack) {
	info->regs.ax = stack->eax;
	info->regs.bx = stack->ebx;
	info->regs.cx = stack->ecx;
	info->regs.dx = stack->edx;
	info->regs.si = stack->esi;
	info->regs.di = stack->edi;
	info->regs.ds = stack->vm86ds;
	info->regs.es = stack->vm86es;
}

static void vm86_copyPtrResult(sVM86Memarea *areas,u16 areaCount) {
	u32 i;
	for(i = 0; i < areaCount; i++) {
		if(areas[i].type == VM86_MEM_PTR) {
			u32 rmAddr = *(u32*)areas[i].data.ptr.srcPtr;
			u32 virt = ((rmAddr & 0xFFFF0000) >> 12) | (rmAddr & 0xFFFF);
			if(virt + areas[i].data.ptr.size > virt && virt + areas[i].data.ptr.size <= 1 * M + 64 * K) {
				if(frameNos[virt / PAGE_SIZE] != virt / PAGE_SIZE) {
					u32 pcount = BYTES_2_PAGES((virt & (PAGE_SIZE - 1)) + areas[i].data.ptr.size);
					paging_map(TEMP_MAP_AREA,frameNos + virt / PAGE_SIZE,pcount,PG_PRESENT | PG_SUPERVISOR);
					memcpy((void*)areas[i].data.ptr.result,
							(void*)(TEMP_MAP_AREA + (virt & (PAGE_SIZE - 1))),
							areas[i].data.ptr.size);
					paging_unmap(TEMP_MAP_AREA,pcount,false);
				}
				else {
					/* note that the first MiB is mapped to 0xC0000000, too */
					memcpy((void*)areas[i].data.ptr.result,
							(void*)(virt | KERNEL_AREA_V_ADDR),areas[i].data.ptr.size);
				}
			}
		}
		else {
			u32 start = areas[i].data.direct.dst / PAGE_SIZE;
			u32 virt = (u32)areas[i].data.direct.src;
			u32 pages = BYTES_2_PAGES((virt & (PAGE_SIZE - 1)) + areas[i].data.direct.size);
			paging_map(TEMP_MAP_AREA,frameNos + start,pages,PG_PRESENT | PG_WRITABLE);
			memcpy((void*)virt,(void*)(TEMP_MAP_AREA + (virt & (PAGE_SIZE - 1))),
					areas[i].data.direct.size);
			paging_unmap(TEMP_MAP_AREA,pages,false);
		}
	}
}

static sVM86Info *vm86_createInfo(u16 interrupt,sVM86Regs *regs,sVM86Memarea *areas,u16 areaCount) {
	sVM86Info *i = (sVM86Info*)kheap_alloc(sizeof(sVM86Info));
	if(i == NULL)
		return NULL;
	i->interrupt = interrupt;
	memcpy(&i->regs,regs,sizeof(sVM86Regs));
	i->areas = NULL;
	i->areaCount = areaCount;
	i->mFrameNos = NULL;
	if(areaCount) {
		i->areas = (sVM86Memarea*)kheap_alloc(areaCount * sizeof(sVM86Memarea));
		if(i->areas == NULL) {
			kheap_free(i);
			return NULL;
		}
		memcpy(i->areas,areas,areaCount * sizeof(sVM86Memarea));
		i->mFrameNos = (u32*)kheap_alloc(areaCount * VM86_MAX_MEMPAGES * sizeof(u32));
		if(i->mFrameNos == NULL) {
			kheap_free(i->areas);
			kheap_free(i);
			return NULL;
		}
	}
	return i;
}

static void vm86_destroyInfo(sVM86Info *i) {
	kheap_free(i->mFrameNos);
	kheap_free(i->areas);
	kheap_free(i);
}
