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

#include <mem/cache.h>
#include <mem/kheap.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <mem/physmemareas.h>
#include <mem/swapmap.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <task/mntspace.h>
#include <task/proc.h>
#include <task/timer.h>
#include <vfs/file.h>
#include <vfs/fs.h>
#include <vfs/info.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <cppsupport.h>
#include <cpu.h>
#include <errno.h>
#include <ostringstream.h>
#include <spinlock.h>
#include <string.h>
#include <util.h>

void VFSInfo::init(VFSNode *sysNode) {
	const fs::User kern = fs::User::kernel();
	VFSNode::release(createObj<MemUsageFile>(kern,sysNode));
	VFSNode::release(createObj<CPUFile>(kern,sysNode));
	VFSNode::release(createObj<StatsFile>(kern,sysNode));
}

void VFSInfo::traceReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Thread *t = getThread(node,dataSize,buffer);
	if(!t)
		return;

	OStringStream os;

	/* we have a thread-ref, which garantees that the process isn't destroyed */
	Proc *p = Proc::getByPid(t->getProc()->getPid());
	if(p) {
		os.writef("Kernel:\n");
		uintptr_t *call = Util::getKernelStackTraceOf(t);
		while(call && *call != 0) {
			KSymbols::Symbol *sym = KSymbols::getSymbolAt(*call);
			os.writef("\t%p (%s)\n",*call,sym ? sym->funcName : "Unknown");
			call++;
		}
		os.writef("User:\n");
		call = Util::getUserStackTraceOf(t);
		while(call && *call != 0) {
			os.writef("\t%p\n",*call);
			call++;
		}
	}
	Thread::relRef(t);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::procReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = getProc(node,dataSize,buffer);
	if(p == NULL)
		return;

	OStringStream os;

	size_t pages,own,shared,swapped;
	p->getVM()->getMemUsage(&pages);
	Proc::getMemUsageOf(p->getPid(),&own,&shared,&swapped);
	os.writef(
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%zu\n"
		"%-16s%lu\n"
		"%-16s%lu\n"
		"%-16s%lu\n"
		"%-16s%lu\n"
		"%-16s%lu\n"
		"%-16s%Lu\n"
		"%-16s%016Lx\n"
		,
		"Pid:",p->getPid(),
		"ParentPid:",p->getParentPid(),
		"Uid:",p->getUid(),
		"Gid:",p->getGid(),
		"Command:",p->getCommand(),
		"Pages:",pages,
		"OwnFrames:",own,
		"SharedFrames:",shared,
		"Swapped:",swapped,
		"Read:",p->getStats().input,
		"Write:",p->getStats().output,
		"Runtime:",p->getRuntime(),
		"Cycles:",p->getStats().lastCycles
	);
	Proc::relRef(p);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::threadReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	ulong stackPages = 0;
	Thread *t = getThread(node,dataSize,buffer);
	if(!t)
		return;

	OStringStream os;

	/* we have a thread-ref, which garantees that the process isn't destroyed */
	Proc *p = Proc::getByPid(t->getProc()->getPid());
	if(p) {
		for(size_t i = 0; i < STACK_REG_COUNT; i++) {
			uintptr_t stackBegin = 0,stackEnd = 0;
			if(t->getStackRange(&stackBegin,&stackEnd,i))
				stackPages += (stackEnd - stackBegin) / PAGE_SIZE;
		}

		os.writef(
			"%-16s%u\n"
			"%-16s%u\n"
			"%-16s%s\n"
			"%-16s%u\n"
			"%-16s%u\n"
			"%-16s%u\n"
			"%-16s%zu\n"
			"%-16s%zu\n"
			"%-16s%zu\n"
			"%-16s%Lu\n"
			"%-16s%016Lx\n"
			"%-16s%u\n"
			,
			"Tid:",t->getTid(),
			"Pid:",p->getPid(),
			"ProcName:",p->getCommand(),
			"State:",t->getState(),
			"Flags:",t->getFlags() & T_IDLE,
			"Priority:",t->getPriority(),
			"StackPages:",stackPages,
			"SchedCount:",t->getStats().schedCount,
			"Syscalls:",t->getStats().syscalls,
			"Runtime:",t->getRuntime(),
			"Cycles:",t->getStats().lastCycleCount,
			"CPU:",t->getCPU()
		);
	}
	Thread::relRef(t);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::cpuReadCallback(A_UNUSED VFSNode *node,size_t *dataSize,void **buffer) {
	OStringStream os;
	CPU::print(os);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::statsReadCallback(A_UNUSED VFSNode *node,size_t *dataSize,void **buffer) {
	OStringStream os;
	uLongLong cycles;
	cycles.val64 = CPU::rdtsc();
	os.writef(
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%016Lx\n"
		"%-16s%zus\n"
		,
		"Processes:",Proc::getCount(),
		"Threads:",Thread::getCount(),
		"Interrupts:",Interrupts::getCount(),
		"CPUCycles:",cycles.val64,
		"UpTime:",Timer::getIntrptCount() / Timer::FREQUENCY_DIV
	);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::memUsageReadCallback(A_UNUSED VFSNode *node,size_t *dataSize,void **buffer) {
	OStringStream os;

	/* TODO change that (kframes, swapping, ...) */
	size_t free = PhysMem::getFreeFrames(PhysMem::DEF | PhysMem::CONT) << PAGE_BITS;
	size_t total = PhysMem::getTotal();
	size_t ksize = Boot::getKernelSize();
	size_t msize = Boot::getModuleSize();
	size_t kheap = KHeap::getOccupiedMem();
	size_t cache = Cache::getOccMem();
	size_t pmem = PhysMem::getStackSize();
	size_t dataShared,dataOwn,dataReal;
	Proc::getMemUsage(&dataShared,&dataOwn,&dataReal);
	os.writef(
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		"%-11s%12zu\n"
		,
		"Total:",total,
		"Used:",total - free,
		"Free:",free,
		"SwapSpace:",SwapMap::totalSpace(),
		"SwapUsed:",SwapMap::totalSpace() - SwapMap::freeSpace(),
		"Kernel:",ksize,
		"Modules:",msize,
		"PhysMem:",pmem,
		"KHeapSize:",kheap,
		"KHeapUsage:",KHeap::getUsedMem(),
		"CacheSize:",cache,
		"CacheUsage:",Cache::getUsedMem(),
		"UserShared:",dataShared,
		"UserOwn:",dataOwn,
		"UserReal:",dataReal
	);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::regionsReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = getProc(node,dataSize,buffer);
	if(!p)
		return;

	OStringStream os;
	p->getVM()->printRegions(os);
	Proc::relRef(p);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::mapsReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = getProc(node,dataSize,buffer);
	if(!p)
		return;

	OStringStream os;
	p->getVM()->printMaps(os);
	Proc::relRef(p);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::virtMemReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = getProc(node,dataSize,buffer);
	if(!p)
		return;

	OStringStream os;
	p->getPageDir()->print(os,PD_PART_USER);
	Proc::relRef(p);
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::selfLinkReadCallback(VFSNode *,size_t *dataSize,void **buffer) {
	Proc *p = Proc::getByPid(Proc::getRunning());
	OStringStream os;
	os.writef("%d",p->getPid());
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

static void buildProcPath(OStringStream &os,Proc *p) {
	if(p->getPid() != 0) {
		Proc *pp = Proc::getRef(p->getParentPid());
		if(pp)
			buildProcPath(os,pp);
		Proc::relRef(pp);
	}
	os.writef("%d/",p->getPid());
}

void VFSInfo::pidLinkReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	VFSNode::acquireTree();
	pid_t pid = node->isAlive() ? atoi(node->name) : INVALID_PID;
	VFSNode::releaseTree();
	if(pid == INVALID_PID)
		return;

	Proc *p = Proc::getRef(pid);
	if(!p)
		return;

	OStringStream os;
	os.writef("../proc/");
	buildProcPath(os,p);
	Proc::relRef(p);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::mountsReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	const VFSMS *msnode = static_cast<VFSMS*>(node->getParent());

	OStringStream os;
	MntSpace *ms = MntSpace::request(msnode->id());
	if(ms) {
		ms->print(os);
		MntSpace::release(ms);
	}
	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSInfo::msLinkReadCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = getProc(node,dataSize,buffer);
	if(!p)
		return;

	OStringStream os;
	os.writef("%s",p->getMS()->getNode()->getPath());
	Proc::relRef(p);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

Proc *VFSInfo::getProc(VFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p = NULL;
	VFSNode::acquireTree();
	if(!node->isAlive() || node->parent->name == NULL) {
		*dataSize = 0;
		*buffer = NULL;
	}
	else {
		pid_t pid = atoi(node->parent->name);
		p = Proc::getRef(pid);
	}
	VFSNode::releaseTree();
	return p;
}

Thread *VFSInfo::getThread(VFSNode *node,size_t *dataSize,void **buffer) {
	Thread *t = NULL;
	VFSNode::acquireTree();
	if(!node->isAlive() || node->parent->name == NULL) {
		*dataSize = 0;
		*buffer = NULL;
	}
	else {
		tid_t tid = atoi(node->parent->name);
		t = Thread::getRef(tid);
	}
	VFSNode::releaseTree();
	return t;
}

ssize_t VFSInfo::readHelper(VFSNode *node,USER void *buffer,off_t offset,size_t count,
		size_t dataSize,read_func callback) {
	void *mem = NULL;
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* just if the datasize is known in advance */
	if(dataSize > 0) {
		/* don't waste time in this case */
		if(offset >= (off_t)dataSize)
			return 0;
		/* ok, use the heap as temporary storage */
		mem = Cache::alloc(dataSize);
		if(mem == NULL)
			return 0;
	}

	/* copy values to public struct */
	callback(node,&dataSize,&mem);
	if(mem == NULL)
		return 0;

	/* correct vars */
	if(offset > (off_t)dataSize)
		offset = dataSize;
	count = esc::Util::min(static_cast<size_t>(dataSize - offset),count);
	/* copy */
	if(count > 0) {
		int res = UserAccess::write(buffer,(uint8_t*)mem + offset,count);
		if(res < 0) {
			Cache::free(mem);
			return res;
		}
	}
	/* free temp storage */
	Cache::free(mem);

	return count;
}
