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
#include <sys/mem/physmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/physmemareas.h>
#include <sys/task/timer.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/file.h>
#include <sys/vfs/fsmsgs.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/printf.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/* callback function for the default read-handler */
typedef void (*fReadCallBack)(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * Creates space, calls the callback which should fill the space
 * with data and writes the corresponding part to the buffer of the user
 */
static ssize_t vfs_info_readHelper(pid_t pid,sVFSNode *node,void *buffer,off_t offset,size_t count,
		size_t dataSize,fReadCallBack callback);

static Proc *vfs_info_getProc(sVFSNode *node,size_t *dataSize,void **buffer);
static pid_t vfs_info_getPid(sVFSNode *node,size_t *dataSize,void **buffer);
static Thread *vfs_info_getThread(sVFSNode *node,size_t *dataSize,void **buffer);

static void vfs_info_traceReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static void vfs_info_procReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static void vfs_info_threadReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static ssize_t vfs_info_cpuReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);
static void vfs_info_cpuReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static ssize_t vfs_info_statsReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);
static void vfs_info_statsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static ssize_t vfs_info_memUsageReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);
static void vfs_info_memUsageReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static void vfs_info_regionsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static void vfs_info_mapsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
static void vfs_info_virtMemReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

void vfs_info_init(void) {
	inode_t nodeNo;
	sVFSNode *sysNode;
	vfs_node_resolvePath("/system",&nodeNo,NULL,VFS_NOACCESS);
	sysNode = vfs_node_get(nodeNo);

	assert(vfs_file_create(KERNEL_PID,sysNode,(char*)"memusage",
			vfs_info_memUsageReadHandler,NULL) != NULL);
	assert(vfs_file_create(KERNEL_PID,sysNode,(char*)"cpu",
			vfs_info_cpuReadHandler,NULL) != NULL);
	assert(vfs_file_create(KERNEL_PID,sysNode,(char*)"stats",
			vfs_info_statsReadHandler,NULL) != NULL);
}

ssize_t vfs_info_traceReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_traceReadCallback);
}

static void vfs_info_traceReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	Util::FuncCall *call;
	sStringBuffer buf;
	Thread *t = vfs_info_getThread(node,dataSize,buffer);
	if(!t)
		return;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	prf_sprintf(&buf,"Kernel:\n");
	call = Util::getKernelStackTraceOf(t);
	while(call && call->addr != 0) {
		prf_sprintf(&buf,"\t%p -> %p (%s)\n",(call + 1)->addr,call->funcAddr,call->funcName);
		call++;
	}
	prf_sprintf(&buf,"User:\n");
	call = Util::getUserStackTraceOf(t);
	while(call && call->addr != 0) {
		prf_sprintf(&buf,"\t%p -> %p\n",(call + 1)->addr,call->funcAddr);
		call++;
	}
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_procReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_procReadCallback);
}

static void vfs_info_procReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	size_t pages,own,shared,swapped;
	Proc *p;
	pid_t pid = vfs_info_getPid(node,dataSize,buffer);
	if(pid == INVALID_PID)
		return;

	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;

	p = Proc::getByPid(pid);
	if(p) {
		p->getVM()->getMemUsage(&pages);
		Proc::getMemUsageOf(pid,&own,&shared,&swapped);
		prf_sprintf(
			&buf,
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
			,
			"Pid:",p->getPid(),
			"ParentPid:",p->getParentPid(),
			"Uid:",p->getEUid(),
			"Gid:",p->getEGid(),
			"Command:",p->getCommand(),
			"Pages:",pages,
			"OwnFrames:",own,
			"SharedFrames:",shared,
			"Swapped:",swapped,
			"Read:",p->stats.input,
			"Write:",p->stats.output
		);
	}
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_threadReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_threadReadCallback);
}

static void vfs_info_threadReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	size_t i;
	ulong stackPages = 0;
	Thread *t = vfs_info_getThread(node,dataSize,buffer);
	if(!t)
		return;

	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		uintptr_t stackBegin = 0,stackEnd = 0;
		if(t->getStackRange(&stackBegin,&stackEnd,i))
			stackPages += (stackEnd - stackBegin) / PAGE_SIZE;
	}

	prf_sprintf(
		&buf,
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
		"Pid:",t->getProc()->getPid(),
		"ProcName:",t->getProc()->getCommand(),
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
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_cpuReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_cpuReadCallback);
}

static void vfs_info_cpuReadCallback(A_UNUSED sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	CPU::sprintf(&buf);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_statsReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_statsReadCallback);
}

static void vfs_info_statsReadCallback(A_UNUSED sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	uLongLong cycles;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;

	cycles.val64 = CPU::rdtsc();
	prf_sprintf(
		&buf,
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
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_memUsageReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,
		USER void *buffer,off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_memUsageReadCallback);
}

static void vfs_info_memUsageReadCallback(A_UNUSED sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	size_t free,total,dataShared,dataOwn,dataReal,ksize,msize,kheap,cache,pmem;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;

	/* TODO change that (kframes, swapping, ...) */
	free = PhysMem::getFreeFrames(PhysMem::DEF | PhysMem::CONT) << PAGE_SIZE_SHIFT;
	total = PhysMemAreas::getTotal();
	ksize = Boot::getKernelSize();
	msize = Boot::getModuleSize();
	kheap = KHeap::getOccupiedMem();
	cache = Cache::getOccMem();
	pmem = PhysMem::getStackSize();
	Proc::getMemUsage(&dataShared,&dataOwn,&dataReal);
	prf_sprintf(
		&buf,
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
		"%-11s%10zu\n"
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
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_regionsReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_regionsReadCallback);
}

static void vfs_info_regionsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	pid_t pid = vfs_info_getPid(node,dataSize,buffer);
	if(pid == INVALID_PID)
		return;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	Proc::getByPid(pid)->getVM()->sprintfRegions(&buf);
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_mapsReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_mapsReadCallback);
}

static void vfs_info_mapsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	pid_t pid = vfs_info_getPid(node,dataSize,buffer);
	if(pid == INVALID_PID)
		return;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	Proc::getByPid(pid)->getVM()->sprintfMaps(&buf);
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_virtMemReadHandler(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_virtMemReadCallback);
}

static void vfs_info_virtMemReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	Proc *p = vfs_info_getProc(node,dataSize,buffer);
	if(!p)
		return;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p->getPageDir()->sprintf(&buf);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static Proc *vfs_info_getProc(sVFSNode *node,size_t *dataSize,void **buffer) {
	Proc *p;
	SpinLock::aquire(&node->lock);
	if(node->name == NULL) {
		*dataSize = 0;
		*buffer = NULL;
		SpinLock::release(&node->lock);
		return NULL;
	}
	p = Proc::getByPid(atoi(node->parent->name));
	SpinLock::release(&node->lock);
	return p;
}

static pid_t vfs_info_getPid(sVFSNode *node,size_t *dataSize,void **buffer) {
	pid_t pid;
	SpinLock::aquire(&node->lock);
	if(node->name == NULL) {
		*dataSize = 0;
		*buffer = NULL;
		SpinLock::release(&node->lock);
		return INVALID_PID;
	}
	pid = atoi(node->parent->name);
	SpinLock::release(&node->lock);
	return pid;
}

static Thread *vfs_info_getThread(sVFSNode *node,size_t *dataSize,void **buffer) {
	Thread *t;
	SpinLock::aquire(&node->lock);
	if(node->name == NULL) {
		*dataSize = 0;
		*buffer = NULL;
		SpinLock::release(&node->lock);
		return NULL;
	}
	t = Thread::getById(atoi(node->parent->name));
	SpinLock::release(&node->lock);
	return t;
}

static ssize_t vfs_info_readHelper(A_UNUSED pid_t pid,sVFSNode *node,USER void *buffer,off_t offset,
		size_t count,size_t dataSize,fReadCallBack callback) {
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
	count = MIN(dataSize - offset,count);
	/* copy */
	if(count > 0) {
		Thread::addHeapAlloc(mem);
		memcpy(buffer,(uint8_t*)mem + offset,count);
		Thread::remHeapAlloc(mem);
	}
	/* free temp storage */
	Cache::free(mem);

	return count;
}
