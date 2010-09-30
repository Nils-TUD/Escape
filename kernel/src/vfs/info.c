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

#include <sys/common.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/machine/timer.h>
#include <sys/machine/cpu.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/rw.h>
#include <sys/vfs/real.h>
#include <sys/multiboot.h>
#include <sys/util.h>
#include <sys/printf.h>
#include <assert.h>
#include <string.h>

/* VFS-directory-entry (equal to the direntry of ext2) */
typedef struct {
	tInodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} A_PACKED sVFSDirEntry;

/**
 * The read-callback for the trace-read-handler
 */
static void vfsinfo_traceReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);
/**
 * The read-callback for the proc-read-handler
 */
static void vfsinfo_procReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-callback for the thread-read-handler
 */
static void vfsinfo_threadReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The cpu-read-handler
 */
static s32 vfsinfo_cpuReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the cpu-read-handler
 */
static void vfsinfo_cpuReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The stats-read-handler
 */
static s32 vfsinfo_statsReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the stats-read-handler
 */
static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-handler for the mem-usage-node
 */
static s32 vfsinfo_memUsageReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the VFS memusage-read-handler
 */
static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-callback for the regions-read-handler
 */
static void vfsinfo_regionsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-callback for the virtual-memory-read-handler
 */
static void vfsinfo_virtMemReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

void vfsinfo_init(void) {
	tInodeNo nodeNo;
	sVFSNode *sysNode;
	vfsn_resolvePath("/system",&nodeNo,NULL,VFS_NOACCESS);
	sysNode = vfsn_getNode(nodeNo);

	assert(vfsn_createFile(KERNEL_PID,sysNode,(char*)"memusage",
			vfsinfo_memUsageReadHandler,NULL,true) != NULL);
	assert(vfsn_createFile(KERNEL_PID,sysNode,(char*)"cpu",
			vfsinfo_cpuReadHandler,NULL,true) != NULL);
	assert(vfsn_createFile(KERNEL_PID,sysNode,(char*)"stats",
			vfsinfo_statsReadHandler,NULL,true) != NULL);
}

s32 vfsinfo_traceReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,0,vfsinfo_traceReadCallback);
}

static void vfsinfo_traceReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sThread *t = thread_getById(atoi(node->parent->name));
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
		prf_sprintf(&buf,"\t%#08x -> %#08x (%s)\n",(call + 1)->addr,call->funcAddr,call->funcName);
		call++;
	}
	prf_sprintf(&buf,"User:\n");
	call = util_getUserStackTraceOf(t);
	while(call && call->addr != 0) {
		prf_sprintf(&buf,"\t%#08x -> %#08x\n",(call + 1)->addr,call->funcAddr);
		call++;
	}
	*buffer = buf.str;
	*dataSize = buf.len;
}

s32 vfsinfo_procReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	/* don't use the cache here to prevent that one process occupies it for all others */
	/* (if the process doesn't call close() the cache will not be invalidated and therefore
	 * other processes might miss changes) */
	return vfsrw_readHelper(pid,node,buffer,offset,count,
			17 * 9 + 8 * 10 + MAX_PROC_NAME_LEN + 1,
			vfsinfo_procReadCallback);
}

static void vfsinfo_procReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sProc *p = proc_getByPid(atoi(node->parent->name));
	sStringBuffer buf;
	u32 pages;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 9 + 8 * 10 + MAX_PROC_NAME_LEN + 1;
	buf.len = 0;
	vmm_getMemUsage(p,&pages);

	prf_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		,
		"Pid:",p->pid,
		"ParentPid:",p->parentPid,
		"Command:",p->command,
		"Pages:",pages,
		"OwnFrames:",p->ownFrames,
		"SharedFrames:",p->sharedFrames,
		"Swapped:",p->swapped,
		"Read:",p->stats.input,
		"Write:",p->stats.output
	);
	*dataSize = buf.len;
}

s32 vfsinfo_threadReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,
			17 * 8 + 6 * 10 + 2 * 16 + 1,vfsinfo_threadReadCallback);
}

static void vfsinfo_threadReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sThread *t = thread_getById(atoi(node->parent->name));
	sStringBuffer buf;
	u32 stackBegin = 0,stackEnd = 0;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 8 + 6 * 10 + 2 * 16 + 1;
	buf.len = 0;
	if(t->stackRegion >= 0)
		vmm_getRegRange(t->proc,t->stackRegion,&stackBegin,&stackEnd);

	prf_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%08x%08x\n"
		"%-16s%08x%08x\n"
		,
		"Tid:",t->tid,
		"Pid:",t->proc->pid,
		"State:",t->state,
		"StackPages:",(stackEnd - stackBegin) / PAGE_SIZE,
		"SchedCount:",t->stats.schedCount,
		"Syscalls:",t->stats.syscalls,
		"UCPUCycles:",t->stats.ucycleCount.val32.upper,t->stats.ucycleCount.val32.lower,
		"KCPUCycles:",t->stats.kcycleCount.val32.upper,t->stats.kcycleCount.val32.lower
	);
	*dataSize = buf.len;
}

static s32 vfsinfo_cpuReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,0,vfsinfo_cpuReadCallback);
}

static void vfsinfo_cpuReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
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

static s32 vfsinfo_statsReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,0,vfsinfo_statsReadCallback);
}

static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
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
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%08x%08x\n"
		"%-16s%us\n"
		,
		"Processes:",proc_getCount(),
		"Threads:",thread_getCount(),
		"Interrupts:",intrpt_getCount(),
		"CPUCycles:",cycles.val32.upper,cycles.val32.lower,
		"UpTime:",timer_getIntrptCount() / TIMER_FREQUENCY
	);
	*buffer = buf.str;
	*dataSize = buf.len;
}

static s32 vfsinfo_memUsageReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,(11 + 10 + 1) * 13 + 1,
			vfsinfo_memUsageReadCallback);
}

static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	u32 free,total;
	u32 paging,dataShared,dataOwn,dataReal,ksize,msize,kheap,pmem;
	UNUSED(node);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = (11 + 10 + 1) * 13 + 1;
	buf.len = 0;

	free = mm_getFreeFrames(MM_DEF | MM_CONT) << PAGE_SIZE_SHIFT;
	total = mboot_getUsableMemCount();
	ksize = mboot_getKernelSize();
	msize = mboot_getModuleSize();
	kheap = kheap_getOccupiedMem();
	pmem = mm_getStackSize();
	proc_getMemUsage(&paging,&dataShared,&dataOwn,&dataReal);
	prf_sprintf(
		&buf,
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
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
		"KernelMisc:",(total - free) - (ksize + msize + dataReal + paging + pmem + kheap)
	);
	*dataSize = buf.len;
}

s32 vfsinfo_regionsReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,0,vfsinfo_regionsReadCallback);
}

static void vfsinfo_regionsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	sProc *p;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p = proc_getByPid(atoi(node->parent->name));
	vmm_sprintfRegions(&buf,p);
	*buffer = buf.str;
	*dataSize = buf.len;
}

s32 vfsinfo_virtMemReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(pid,node,buffer,offset,count,0,vfsinfo_virtMemReadCallback);
}

static void vfsinfo_virtMemReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	sProc *p;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p = proc_getByPid(atoi(node->parent->name));
	paging_sprintfVirtMem(&buf,p->pagedir);
	*buffer = buf.str;
	*dataSize = buf.len;
}

s32 vfsinfo_dirReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount,fsByteCount;

	UNUSED(file);
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* TODO should we cache the fs-accesses for the root-dir, too? */

	/* not cached yet? */
	if(node->data.def.cache == NULL) {
		/* we need the number of bytes first */
		u8 *fsBytes = NULL,*fsBytesDup;
		sVFSNode *n = vfsn_getFirstChild(node);
		byteCount = 0;
		fsByteCount = 0;
		while(n != NULL) {
			if(node->parent != NULL || (strcmp(n->name,".") != 0 && strcmp(n->name,"..") != 0))
				byteCount += sizeof(sVFSDirEntry) + strlen(n->name);
			n = n->next;
		}

		/* the root-directory is distributed on the fs-driver and the kernel */
		/* therefore we have to read it from the fs-driver, too */
		/* but don't do that if we're the kernel (vfsr does not work then) */
		if(node->parent == NULL && pid != KERNEL_PID) {
			const u32 bufSize = 1024;
			u32 c,curSize = bufSize;
			fsBytes = (u8*)kheap_alloc(bufSize);
			if(fsBytes != NULL) {
				tFileNo rfile = vfsr_openFile(pid,VFS_READ,"/");
				if(rfile >= 0) {
					while((c = vfs_readFile(pid,rfile,fsBytes + fsByteCount,bufSize)) > 0) {
						fsByteCount += c;
						if(c < bufSize)
							break;

						curSize += bufSize;
						fsBytesDup = kheap_realloc(fsBytes,curSize);
						if(fsBytesDup == NULL) {
							kheap_free(fsBytes);
							fsBytes = NULL;
							break;
						}
						fsBytes = fsBytesDup;
					}
					vfs_closeFile(pid,rfile);
				}
			}
			byteCount += fsByteCount;
		}

		node->data.def.size = byteCount;
		node->data.def.pos = byteCount;
		if(byteCount > 0) {
			u8 *childs;
			/* now allocate mem on the heap and copy all data into it */
			if(fsBytes)
				childs = kheap_realloc(fsBytes,byteCount);
			else
				childs = (u8*)kheap_alloc(byteCount);

			if(childs == NULL) {
				if(fsBytes)
					kheap_free(fsBytes);
				node->data.def.size = 0;
				node->data.def.pos = 0;
			}
			else {
				u16 len;
				sVFSDirEntry *dirEntry = (sVFSDirEntry*)(childs + fsByteCount);
				node->data.def.cache = childs;
				n = vfsn_getFirstChild(node);
				while(n != NULL) {
					if(node->parent == NULL && (strcmp(n->name,".") == 0 || strcmp(n->name,"..") == 0)) {
						n = n->next;
						continue;
					}
					len = strlen(n->name);
					dirEntry->nodeNo = vfsn_getNodeNo(n);
					dirEntry->nameLen = len;
					dirEntry->recLen = sizeof(sVFSDirEntry) + len;
					memcpy(dirEntry + 1,n->name,len);
					dirEntry = (sVFSDirEntry*)((u8*)dirEntry + dirEntry->recLen);
					n = n->next;
				}
			}
		}
	}

	if(offset > node->data.def.size)
		offset = node->data.def.size;
	byteCount = MIN(node->data.def.size - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.def.cache + offset,byteCount);
	}

	/* no cache for root-folder */
	if(node->parent == NULL) {
		node->data.def.size = 0;
		node->data.def.pos = 0;
		kheap_free(node->data.def.cache);
		node->data.def.cache = NULL;
	}

	return byteCount;
}
