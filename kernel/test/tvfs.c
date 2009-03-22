/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "../h/proc.h"
#include "../h/kheap.h"
#include "../h/video.h"
#include <string.h>

#include "tvfs.h"
#include <test.h>

/* forward declarations */
static void test_vfs(void);
static void test_vfs_readFileSystem(void);
static void test_vfs_readFileProcess0(void);
static void test_vfs_createProcess(void);
static void test_vfs_createService(void);

/* public vfs-node for test-purposes */
typedef struct {
	tVFSNodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} __attribute__((packed)) sVFSDirEntry;
typedef struct {
	sVFSDirEntry header;
	s8 name[MAX_NAME_LEN + 1];
} __attribute__((packed)) sVFSDirEntryRead;

/* public process-data for test-purposes */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
} sProcPub;

/* our test-module */
sTestModule tModVFS = {
	"VFS",
	&test_vfs
};

static void test_vfs(void) {
	test_vfs_readFileSystem();
	test_vfs_readFileProcess0();
	test_vfs_createProcess();
	test_vfs_createService();
}

static void test_vfs_readFileSystem(void) {
	tVFSNodeNo nodeNo,procNode;
	s32 fd;
	s32 res;
	tFile file;
	sVFSDirEntryRead node;
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_dbg_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for system:");

	/* resolve path */
	if(vfsn_resolvePath("system:",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path 'system:'!");
		return;
	}

	/* get free fd */
	fd = proc_getFreeFd();
	if(fd < 0) {
		test_caseFailed("Unable to get free fd!");
		return;
	}

	/* open */
	file = vfs_openFile(KERNEL_PID,VFS_READ,nodeNo);
	if(file < 0) {
		test_caseFailed("Unable to open 'system:'!");
		return;
	}

	/* assoc fd with file */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		test_caseFailed("Unable to assoc fd!");
		return;
	}

	/* skip "." and ".." */
	vfs_readFile(KERNEL_PID,file,(u8*)&node,sizeof(sVFSDirEntry));
	vfs_readFile(KERNEL_PID,file,(u8*)node.name,node.header.nameLen);
	vfs_readFile(KERNEL_PID,file,(u8*)&node,sizeof(sVFSDirEntry));
	vfs_readFile(KERNEL_PID,file,(u8*)node.name,node.header.nameLen);

	/* read "processes" */
	if((res = vfs_readFile(KERNEL_PID,file,(u8*)&node,sizeof(sVFSDirEntry))) != sizeof(sVFSDirEntry)) {
		proc_unassocFD(fd);
		vfs_closeFile(file);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sVFSDirEntry),res);
		return;
	}
	vfs_readFile(KERNEL_PID,file,(u8*)node.name,node.header.nameLen);
	node.name[node.header.nameLen] = '\0';

	/* check data */
	vfsn_resolvePath("system:/processes",&procNode);
	if(strcmp((cstring)node.name,"processes") != 0) {
		proc_unassocFD(fd);
		vfs_closeFile(file);
		test_caseFailed("Node-name='%s', expected 'processes'",node.name);
		return;
	}
	if(node.header.nodeNo != procNode) {
		proc_unassocFD(fd);
		vfs_closeFile(file);
		test_caseFailed("nodeNo=%d, expected %d",node.header.nodeNo);
		return;
	}

	/* unassoc fd */
	tFile fileNo = proc_unassocFD(fd);
	if(fileNo < 0) {
		vfs_closeFile(file);
		test_caseFailed("Unable to unassoc the fd!");
		return;
	}

	/* close */
	vfs_closeFile(file);

	newHeap = kheap_getFreeMem();
	newGFT = vfs_dbg_getGFTEntryCount();
	if(oldHeap != newHeap || oldGFT != newGFT) {
		test_caseFailed("oldHeap=%d, newHeap=%d, oldGFT=%d, newGFT=%d",oldHeap,newHeap,oldGFT,newGFT);
		return;
	}

	test_caseSucceded();
}

static void test_vfs_readFileProcess0(void) {
	tVFSNodeNo nodeNo;
	s32 fd;
	s32 res;
	tFile file;
	sProcPub proc;
	sProc *p0 = proc_getByPid(0);
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_dbg_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for system:/processes/0");

	/* resolve path */
	if(vfsn_resolvePath("system:/processes/0",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path 'system:/processes/0'!");
		return;
	}

	/* get free fd */
	fd = proc_getFreeFd();
	if(fd < 0) {
		test_caseFailed("Unable to get free fd!");
		return;
	}

	/* open */
	file = vfs_openFile(KERNEL_PID,VFS_READ,nodeNo);
	if(file < 0) {
		test_caseFailed("Unable to open 'system:/processes/0'!");
		return;
	}

	/* assoc fd with file */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		test_caseFailed("Unable to assoc fd!");
		return;
	}

	/* read */
	if((res = vfs_readFile(KERNEL_PID,file,(u8*)&proc,sizeof(sProcPub))) != sizeof(sProcPub)) {
		proc_unassocFD(fd);
		vfs_closeFile(file);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sProcPub),res);
		return;
	}

	/* check data */
	if(!test_assertInt(proc.textPages,p0->textPages)) {proc_unassocFD(fd); vfs_closeFile(file); return;}
	if(!test_assertInt(proc.dataPages,p0->dataPages)) {proc_unassocFD(fd); vfs_closeFile(file); return;}
	if(!test_assertInt(proc.stackPages,p0->stackPages)) {proc_unassocFD(fd); vfs_closeFile(file); return;}
	if(!test_assertInt(proc.pid,p0->pid)) {proc_unassocFD(fd); vfs_closeFile(file); return;}
	if(!test_assertInt(proc.parentPid,p0->parentPid)) {proc_unassocFD(fd); vfs_closeFile(file); return;}
	if(!test_assertInt(proc.state,p0->state)) {proc_unassocFD(fd); vfs_closeFile(file); return;}

	/* unassoc fd */
	tFile fileNo = proc_unassocFD(fd);
	if(fileNo < 0) {
		vfs_closeFile(file);
		test_caseFailed("Unable to unassoc the fd!");
		return;
	}

	/* close */
	vfs_closeFile(file);

	newHeap = kheap_getFreeMem();
	newGFT = vfs_dbg_getGFTEntryCount();
	if(oldHeap != newHeap || oldGFT != newGFT) {
		test_caseFailed("oldHeap=%d, newHeap=%d, oldGFT=%d, newGFT=%d",oldHeap,newHeap,oldGFT,newGFT);
		return;
	}

	test_caseSucceded();
}

static s32 dummyReadHandler(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(pid);
	UNUSED(node);
	UNUSED(buffer);
	UNUSED(offset);
	UNUSED(count);
	return 0;
}

static void test_vfs_createProcess(void) {
	u32 oldHeap,newHeap;

	test_caseStart("Testing vfs_createProcess()");

	oldHeap = kheap_getFreeMem();

	/* create */
	if(!vfs_createProcess(123,&dummyReadHandler)) {
		test_caseFailed("Unable to create node with process id 123");
		return;
	}
	/* try another create with same pid */
	if(vfs_createProcess(123,&dummyReadHandler)) {
		test_caseFailed("Created another node with pid 123");
		return;
	}
	/* remove */
	vfs_removeProcess(123);

	/* check mem-usage */
	newHeap = kheap_getFreeMem();
	if(oldHeap != newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}

static void test_vfs_createService(void) {
	u32 oldHeap,newHeap;
	s32 id,id2;

	test_caseStart("Testing vfs_createService()");

	oldHeap = kheap_getFreeMem();

	id = vfs_createService(0,"test",0);
	if(!test_assertTrue(vfsn_isValidNodeNo(id))) return;
	if(!test_assertInt(vfs_createService(0,"test2",0),ERR_PROC_DUP_SERVICE)) return;
	if(!test_assertInt(vfs_createService(1,"test",0),ERR_SERVICE_EXISTS)) return;
	if(!test_assertInt(vfs_createService(1,"",0),ERR_INV_SERVICE_NAME)) return;
	if(!test_assertInt(vfs_createService(1,"abc.def",0),ERR_INV_SERVICE_NAME)) return;
	id2 = vfs_createService(1,"test2",0);
	if(!test_assertTrue(vfsn_isValidNodeNo(id2))) return;

	vfs_removeService(0,id);
	vfs_removeService(1,id2);

	/* check mem-usage */
	newHeap = kheap_getFreeMem();
	if(oldHeap != newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}
