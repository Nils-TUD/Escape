/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <mm.h>
#include <multiboot.h>
#include <proc.h>
#include <vfs.h>
#include <vfsnode.h>
#include <vfsinfo.h>
#include <util.h>
#include <kheap.h>
#include <string.h>

/**
 * The read-callback for the proc-read-handler
 */
static void vfsinfo_procReadCallback(sVFSNode *node,void *buffer);

/**
 * The read-handler for the mem-usage-node
 */
static s32 vfsinfo_memUsageReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the VFS memusage-read-handler
 */
static void vfsinfo_memUsageReadCallback(sVFSNode *node,void *buffer);

void vfsinfo_init(void) {
	tVFSNodeNo nodeNo;
	vfsn_resolvePath("system:/",&nodeNo);
	vfsn_createInfo(KERNEL_PID,vfsn_getNode(nodeNo),(char*)"memusage",vfsinfo_memUsageReadHandler);
}

s32 vfsinfo_procReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	/* don't use the cache here to prevent that one process occupies it for all others */
	/* (if the process doesn't call close() the cache will not be invalidated and therefore
	 * other processes might miss changes) */
	return vfs_readHelper(pid,node,buffer,offset,count,
			18 * 9 + 6 * 10 + 2 * 16 + MAX_PROC_NAME_LEN + 1,
			vfsinfo_procReadCallback);
}

static void vfsinfo_procReadCallback(sVFSNode *node,void *buffer) {
	char *str = (char*)buffer;
	sProc *p = proc_getByPid(atoi(node->name));
	u32 *uptr,*kptr;
	UNUSED(node);

	uptr = (u32*)&p->ucycleCount;
	kptr = (u32*)&p->kcycleCount;
	util_sprintf(
		str,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%08x%08x\n"
		"%-16s%08x%08x\n"
		,
		"Pid:",p->pid,
		"ParentPid:",p->parentPid,
		"Command:",p->command,
		"State:",p->state,
		"TextPages:",p->textPages,
		"DataPages:",p->dataPages,
		"StackPages:",p->stackPages,
		"UCPUCycles:",*(uptr + 1),*uptr,
		"KCPUCycles:",*(kptr + 1),*kptr
	);
}

static s32 vfsinfo_memUsageReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(pid,node,buffer,offset,count,(8 + 1 + 10 + 1) * 4 + 1,
			vfsinfo_memUsageReadCallback);
}

static void vfsinfo_memUsageReadCallback(sVFSNode *node,void *buffer) {
	char *str = (char*)buffer;
	UNUSED(node);

	u32 free = mm_getFreeFrmCount(MM_DEF | MM_DMA) << PAGE_SIZE_SHIFT;
	u32 total = mboot_getUsableMemCount();
	util_sprintf(
		str,
		"%-9s%10u\n"
		"%-9s%10u\n"
		"%-9s%10u\n"
		"%-9s%10u\n"
		,
		"Total:",total,
		"Used:",total - free,
		"Free:",free,
		"KHeap:",kheap_getFreeMem()
	);
}
