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
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/task/timer.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/file.h>
#include <sys/vfs/real.h>
#include <sys/cpu.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/printf.h>
#include <assert.h>
#include <string.h>

/* callback function for the default read-handler */
typedef void (*fReadCallBack)(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * Creates space, calls the callback which should fill the space
 * with data and writes the corresponding part to the buffer of the user
 */
static ssize_t vfs_info_readHelper(pid_t pid,sVFSNode *node,void *buffer,off_t offset,size_t count,
		size_t dataSize,fReadCallBack callback);

/**
 * The read-callback for the trace-read-handler
 */
static void vfs_info_traceReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);
/**
 * The read-callback for the proc-read-handler
 */
static void vfs_info_procReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The read-callback for the thread-read-handler
 */
static void vfs_info_threadReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The cpu-read-handler
 */
static ssize_t vfs_info_cpuReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The read-callback for the cpu-read-handler
 */
static void vfs_info_cpuReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The stats-read-handler
 */
static ssize_t vfs_info_statsReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The read-callback for the stats-read-handler
 */
static void vfs_info_statsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The read-handler for the mem-usage-node
 */
static ssize_t vfs_info_memUsageReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The read-callback for the VFS memusage-read-handler
 */
static void vfs_info_memUsageReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The read-callback for the regions-read-handler
 */
static void vfs_info_regionsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer);

/**
 * The read-callback for the virtual-memory-read-handler
 */
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

ssize_t vfs_info_traceReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_traceReadCallback);
}

static void vfs_info_traceReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	const sThread *t = thread_getById(atoi(node->parent->name));
	sFuncCall *call;
	sStringBuffer buf;
	UNUSED(dataSize);
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	prf_sprintf(&buf,"Kernel:\n");
	call = util_getKernelStackTraceOf(t);
	while(call && call->addr != 0) {
		prf_sprintf(&buf,"\t%p -> %p (%s)\n",(call + 1)->addr,call->funcAddr,call->funcName);
		call++;
	}
	prf_sprintf(&buf,"User:\n");
	call = util_getUserStackTraceOf(t);
	while(call && call->addr != 0) {
		prf_sprintf(&buf,"\t%p -> %p\n",(call + 1)->addr,call->funcAddr);
		call++;
	}
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_procReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_procReadCallback);
}

static void vfs_info_procReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	const sProc *p = proc_getByPid(atoi(node->parent->name));
	sStringBuffer buf;
	size_t pages;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	vmm_getMemUsage(p,&pages);

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
		"Pid:",p->pid,
		"ParentPid:",p->parentPid,
		"Uid:",p->euid,
		"Gid:",p->egid,
		"Command:",p->command,
		"Pages:",pages,
		"OwnFrames:",p->ownFrames,
		"SharedFrames:",p->sharedFrames,
		"Swapped:",p->swapped,
		"Read:",p->stats.input,
		"Write:",p->stats.output
	);
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_threadReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_threadReadCallback);
}

static void vfs_info_threadReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	const sThread *t = thread_getById(atoi(node->parent->name));
	sStringBuffer buf;
	size_t i;
	ulong stackPages = 0;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		uintptr_t stackBegin = 0,stackEnd = 0;
		if(thread_getStackRange(t,&stackBegin,&stackEnd,i))
			stackPages += (stackEnd - stackBegin) / PAGE_SIZE;
	}

	prf_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%u\n"
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%016Lx\n"
		"%-16s%016Lx\n"
		,
		"Tid:",t->tid,
		"Pid:",t->proc->pid,
		"ProcName:",t->proc->command,
		"State:",t->state,
		"StackPages:",stackPages,
		"SchedCount:",t->stats.schedCount,
		"Syscalls:",t->stats.syscalls,
		"UCPUCycles:",t->stats.ucycleCount.val64,
		"KCPUCycles:",t->stats.kcycleCount.val64
	);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_cpuReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_cpuReadCallback);
}

static void vfs_info_cpuReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	UNUSED(node);
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	cpu_sprintf(&buf);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_statsReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_statsReadCallback);
}

static void vfs_info_statsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	uLongLong cycles;
	UNUSED(dataSize);
	UNUSED(node);
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;

	cycles.val64 = cpu_rdtsc();
	prf_sprintf(
		&buf,
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%zu\n"
		"%-16s%016Lx\n"
		"%-16s%zus\n"
		,
		"Processes:",proc_getCount(),
		"Threads:",thread_getCount(),
		"Interrupts:",intrpt_getCount(),
		"CPUCycles:",cycles.val64,
		"UpTime:",timer_getIntrptCount() / TIMER_FREQUENCY
	);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_memUsageReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,(11 + 10 + 1) * 15 + 1,
			vfs_info_memUsageReadCallback);
}

static void vfs_info_memUsageReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	size_t free,total;
	size_t paging,dataShared,dataOwn,dataReal,ksize,msize,kheap,cache,pmem;
	UNUSED(node);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = (11 + 10 + 1) * 15 + 1;
	buf.len = 0;

	free = pmem_getFreeFrames(MM_DEF | MM_CONT) << PAGE_SIZE_SHIFT;
	total = boot_getUsableMemCount();
	ksize = boot_getKernelSize();
	msize = boot_getModuleSize();
	kheap = kheap_getOccupiedMem();
	cache = cache_getOccMem();
	pmem = pmem_getStackSize();
	proc_getMemUsage(&paging,&dataShared,&dataOwn,&dataReal);
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
		"Kernel:",ksize,
		"Modules:",msize,
		"UserShared:",dataShared,
		"UserOwn:",dataOwn,
		"UserReal:",dataReal,
		"Paging:",paging,
		"PhysMem:",pmem,
		"KHeapSize:",kheap,
		"KHeapUsage:",kheap_getUsedMem(),
		"CacheSize:",cache,
		"CacheUsage:",cache_getUsedMem(),
		"KernelMisc:",(total - free) - (ksize + msize + dataReal + paging + pmem + kheap + cache)
	);
	*dataSize = buf.len;
}

ssize_t vfs_info_regionsReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_regionsReadCallback);
}

static void vfs_info_regionsReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	const sProc *p;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p = proc_getByPid(atoi(node->parent->name));
	vmm_sprintfRegions(&buf,p);
	*buffer = buf.str;
	*dataSize = buf.len;
}

ssize_t vfs_info_virtMemReadHandler(pid_t pid,file_t file,sVFSNode *node,void *buffer,
		off_t offset,size_t count) {
	UNUSED(file);
	return vfs_info_readHelper(pid,node,buffer,offset,count,0,vfs_info_virtMemReadCallback);
}

static void vfs_info_virtMemReadCallback(sVFSNode *node,size_t *dataSize,void **buffer) {
	sStringBuffer buf;
	const sProc *p;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p = proc_getByPid(atoi(node->parent->name));
	paging_sprintfVirtMem(&buf,p->pagedir);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static ssize_t vfs_info_readHelper(pid_t pid,sVFSNode *node,void *buffer,off_t offset,size_t count,
		size_t dataSize,fReadCallBack callback) {
	void *mem = NULL;

	UNUSED(pid);
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* just if the datasize is known in advance */
	if(dataSize > 0) {
		/* can we copy it directly? */
		if(offset == 0 && count == dataSize)
			mem = buffer;
		/* don't waste time in this case */
		else if(offset >= (off_t)dataSize)
			return 0;
		/* ok, use the heap as temporary storage */
		else {
			mem = cache_alloc(dataSize);
			if(mem == NULL)
				return 0;
		}
	}

	/* copy values to public struct */
	callback(node,&dataSize,&mem);
	if(mem == NULL)
		return 0;

	/* stored on kheap? */
	if((uintptr_t)mem != (uintptr_t)buffer) {
		/* correct vars */
		if(offset > (off_t)dataSize)
			offset = dataSize;
		count = MIN(dataSize - offset,count);
		/* copy */
		if(count > 0)
			memcpy(buffer,(uint8_t*)mem + offset,count);
		/* free temp storage */
		cache_free(mem);
	}

	return count;
}
