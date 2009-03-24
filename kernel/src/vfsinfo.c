/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/mm.h"
#include "../h/multiboot.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "../h/vfsinfo.h"
#include <string.h>

/* public process-data */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	u64 cycleCount;
	char command[MAX_PROC_NAME_LEN + 1];
} sProcPub;

/* public memusage-data */
typedef struct {
	u32 totalMem;
	u32 freeMem;
} sMemUsage;

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
	vfsn_createInfo(vfsn_getNode(nodeNo),(char*)"memusage",vfsinfo_memUsageReadHandler);
}

s32 vfsinfo_procReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	/* don't use the cache here to prevent that one process occupies it for all others */
	/* (if the process doesn't call close() the cache will not be invalidated and therefore
	 * other processes might miss changes) */
	return vfs_readHelper(pid,node,buffer,offset,count,sizeof(sProcPub),vfsinfo_procReadCallback);
}

static void vfsinfo_procReadCallback(sVFSNode *node,void *buffer) {
	sProc *p = proc_getByPid(atoi(node->name));
	sProcPub *proc = (sProcPub*)buffer;
	proc->state = p->state;
	proc->pid = p->pid;
	proc->parentPid = p->parentPid;
	proc->textPages = p->textPages;
	proc->dataPages = p->dataPages;
	proc->stackPages = p->stackPages;
	proc->cycleCount = p->cycleCount;
	memcpy(proc->command,p->command,strlen(p->command) + 1);
}

static s32 vfsinfo_memUsageReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(pid,node,buffer,offset,count,sizeof(sMemUsage),vfsinfo_memUsageReadCallback);
}

static void vfsinfo_memUsageReadCallback(sVFSNode *node,void *buffer) {
	sMemUsage *mem = (sMemUsage*)buffer;

	UNUSED(node);

	mem->freeMem = mm_getFreeFrmCount(MM_DEF | MM_DMA) << PAGE_SIZE_SHIFT;
	mem->totalMem = mboot_getUsableMemCount();
}
