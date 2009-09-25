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
#include <mem/pmem.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <machine/timer.h>
#include <machine/cpu.h>
#include <task/proc.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/info.h>
#include <vfs/rw.h>
#include <vfs/real.h>
#include <multiboot.h>
#include <util.h>
#include <assert.h>
#include <string.h>
#include <asprintf.h>

/* VFS-directory-entry (equal to the direntry of ext2) */
typedef struct {
	tInodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} __attribute__((packed)) sVFSDirEntry;

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
static s32 vfsinfo_cpuReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the cpu-read-handler
 */
static void vfsinfo_cpuReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The stats-read-handler
 */
static s32 vfsinfo_statsReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the stats-read-handler
 */
static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-handler for the mem-usage-node
 */
static s32 vfsinfo_memUsageReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the VFS memusage-read-handler
 */
static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-callback for the virtual-memory-read-handler
 */
static void vfsinfo_virtMemReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

void vfsinfo_init(void) {
	tInodeNo nodeNo;
	sVFSNode *sysNode;
	vfsn_resolvePath("/system",&nodeNo,NULL,VFS_NOACCESS);
	sysNode = vfsn_getNode(nodeNo);

	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"memusage",vfsinfo_memUsageReadHandler);
	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"cpu",vfsinfo_cpuReadHandler);
	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"stats",vfsinfo_statsReadHandler);
}

s32 vfsinfo_procReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	/* don't use the cache here to prevent that one process occupies it for all others */
	/* (if the process doesn't call close() the cache will not be invalidated and therefore
	 * other processes might miss changes) */
	return vfsrw_readHelper(tid,node,buffer,offset,count,
			17 * 6 + 5 * 10 + MAX_PROC_NAME_LEN + 1,
			vfsinfo_procReadCallback);
}

static void vfsinfo_procReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sProc *p = proc_getByPid(atoi(node->parent->name));
	sStringBuffer buf;
	UNUSED(dataSize);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 6 + 5 * 10 + MAX_PROC_NAME_LEN + 1;
	buf.len = 0;

	asprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		,
		"Pid:",p->pid,
		"ParentPid:",p->parentPid,
		"Command:",p->command,
		"TextPages:",p->textPages,
		"DataPages:",p->dataPages,
		"StackPages:",p->stackPages
	);
}

s32 vfsinfo_threadReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,
			17 * 6 + 4 * 10 + 2 * 16 + 1,vfsinfo_threadReadCallback);
}

static void vfsinfo_threadReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sThread *t = thread_getById(atoi(node->name));
	sStringBuffer buf;
	UNUSED(dataSize);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 6 + 4 * 10 + 2 * 16 + 1;
	buf.len = 0;

	asprintf(
		&buf,
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
		"StackPages:",t->ustackPages,
		"UCPUCycles:",t->ucycleCount.val32.upper,t->ucycleCount.val32.lower,
		"KCPUCycles:",t->kcycleCount.val32.upper,t->kcycleCount.val32.lower
	);
}

static s32 vfsinfo_cpuReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,0,vfsinfo_cpuReadCallback);
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
	*dataSize = buf.len + 1;
}

static s32 vfsinfo_statsReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,17 * 5 + 4 * 10 + 1 + 1 * 16 + 1,
			vfsinfo_statsReadCallback);
}

static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	uLongLong cycles;
	UNUSED(dataSize);
	UNUSED(node);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 5 + 4 * 10 + 1 + 1 * 16 + 1;
	buf.len = 0;

	cycles.val64 = cpu_rdtsc();
	asprintf(
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
}

static s32 vfsinfo_memUsageReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,
		u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,(11 + 10 + 1) * 11 + 1,
			vfsinfo_memUsageReadCallback);
}

static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	u32 free,total;
	u32 paging,userTotal,ksize,msize,kheap,pmem,kptbls;
	UNUSED(node);
	UNUSED(dataSize);
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = (11 + 10 + 1) * 11 + 1;
	buf.len = 0;

	/* kernel-page-tables are shared */
	kptbls = (KERNEL_STACK - (KERNEL_AREA_V_ADDR + (PAGE_SIZE * PT_ENTRY_COUNT))) / PT_ENTRY_COUNT;

	free = mm_getFreeFrmCount(MM_DEF | MM_DMA) << PAGE_SIZE_SHIFT;
	total = mboot_getUsableMemCount();
	ksize = mboot_getKernelSize();
	msize = mboot_getModuleSize();
	kheap = kheap_getOccupiedMem();
	pmem = mm_getStackSize();
	proc_getMemUsage(&paging,&userTotal);
	asprintf(
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
		,
		"Total:",total,
		"Used:",total - free,
		"Free:",free,
		"Kernel:",ksize,
		"Modules:",msize,
		"UserTotal:",userTotal,
		"UserReal:",(total - free) - (ksize + pmem + msize + paging + kheap + kptbls),
		"Paging:",paging,
		"PhysMem:",pmem,
		"KHeapSize:",kheap,
		"KHeapUsage:",kheap_getUsedMem()
	);
}

s32 vfsinfo_virtMemReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,0,vfsinfo_virtMemReadCallback);
}

static void vfsinfo_virtMemReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sStringBuffer buf;
	sProc *p;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	p = proc_getByPid(atoi(node->parent->name));
	paging_sprintfVirtMem(&buf,p);
	*buffer = buf.str;
	*dataSize = buf.len + 1;
}

s32 vfsinfo_dirReadHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount,fsByteCount;

	UNUSED(tid);
	UNUSED(file);
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* TODO should we cache the fs-accesses for the root-dir, too? */

	/* not cached yet? */
	if(node->data.def.cache == NULL) {
		/* we need the number of bytes first */
		u8 *fsBytes = NULL;
		sVFSNode *n = NODE_FIRST_CHILD(node);
		byteCount = 0;
		fsByteCount = 0;
		while(n != NULL) {
			if(node->parent != NULL || (strcmp(n->name,".") != 0 && strcmp(n->name,"..") != 0))
				byteCount += sizeof(sVFSDirEntry) + strlen(n->name);
			n = n->next;
		}

		/* the root-directory is distributed on the fs-service and the kernel */
		/* therefore we have to read it from the fs-service, too */
		if(node->parent == NULL) {
			const u32 bufSize = 1024;
			u32 c,curSize = bufSize;
			fsBytes = (u8*)kheap_alloc(bufSize);
			if(fsBytes != NULL) {
				tFileNo rfile = vfsr_openFile(tid,VFS_READ,"/");
				if(rfile >= 0) {
					while((c = vfs_readFile(tid,rfile,fsBytes + fsByteCount,bufSize)) > 0) {
						fsByteCount += c;
						if(c < bufSize)
							break;

						curSize += bufSize;
						fsBytes = kheap_realloc(fsBytes,curSize);
						if(fsBytes == NULL)
							break;
					}
					vfs_closeFile(tid,rfile);
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
				node->data.def.size = 0;
				node->data.def.pos = 0;
			}
			else {
				u16 len;
				sVFSDirEntry *dirEntry = (sVFSDirEntry*)(childs + fsByteCount);
				node->data.def.cache = childs;
				n = NODE_FIRST_CHILD(node);
				while(n != NULL) {
					if(node->parent == NULL && (strcmp(n->name,".") == 0 || strcmp(n->name,"..") == 0)) {
						n = n->next;
						continue;
					}
					len = strlen(n->name);
					dirEntry->nodeNo = NADDR_TO_VNNO(n);
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
