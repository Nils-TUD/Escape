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
#define MAX_NAME_LEN 59
typedef struct {
	tVFSNodeNo nodeNo;
	u8 name[MAX_NAME_LEN + 1];
} sVFSNodePub;

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
	tFD fd;
	s32 res;
	sVFSNodePub node;
	sGFTEntry *e;
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_dbg_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for system:");

	/* resolve path */
	if(vfsn_resolvePath("system:",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path 'system:'!");
		return;
	}

	/* open */
	if(vfs_openFile(VFS_READ,nodeNo,&fd) < 0) {
		test_caseFailed("Unable to open 'system:'!");
		return;
	}

	/* skip "." and ".." */
	e = (sGFTEntry*)vfs_fdToFile(fd);
	vfs_readFile(e,(u8*)&node,sizeof(sVFSNodePub));
	vfs_readFile(e,(u8*)&node,sizeof(sVFSNodePub));

	/* read "processes" */
	if((res = vfs_readFile(e,(u8*)&node,sizeof(sVFSNodePub))) != sizeof(sVFSNodePub)) {
		vfs_closeFile(e);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sVFSNodePub),res);
		return;
	}

	/* check data */
	vfsn_resolvePath("system:/processes",&procNode);
	if(strcmp((cstring)node.name,"processes") != 0) {
		vfs_closeFile(e);
		test_caseFailed("Node-name='%s', expected 'processes'",node.name);
		return;
	}
	if(node.nodeNo != procNode) {
		vfs_closeFile(e);
		test_caseFailed("nodeNo=%d, expected %d",node.nodeNo);
		return;
	}

	/* close */
	vfs_closeFile(e);

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
	tFD fd;
	s32 res;
	sProcPub proc;
	sProc *p0 = proc_getByPid(0);
	sGFTEntry *e;
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_dbg_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for system:/processes/0");

	/* resolve path */
	if(vfsn_resolvePath("system:/processes/0",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path 'system:/processes/0'!");
		return;
	}

	/* open */
	if(vfs_openFile(VFS_READ,nodeNo,&fd) < 0) {
		test_caseFailed("Unable to open 'system:/processes/0'!");
		return;
	}

	/* read */
	e = (sGFTEntry*)vfs_fdToFile(fd);
	if((res = vfs_readFile(e,(u8*)&proc,sizeof(sProcPub))) != sizeof(sProcPub)) {
		vfs_closeFile(e);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sProcPub),res);
		return;
	}

	/* check data */
	if(!test_assertInt(proc.textPages,p0->textPages)) {vfs_closeFile(e); return;}
	if(!test_assertInt(proc.dataPages,p0->dataPages)) {vfs_closeFile(e); return;}
	if(!test_assertInt(proc.stackPages,p0->stackPages)) {vfs_closeFile(e); return;}
	if(!test_assertInt(proc.pid,p0->pid)) {vfs_closeFile(e); return;}
	if(!test_assertInt(proc.parentPid,p0->parentPid)) {vfs_closeFile(e); return;}
	if(!test_assertInt(proc.state,p0->state)) {vfs_closeFile(e); return;}

	/* close */
	vfs_closeFile(e);

	newHeap = kheap_getFreeMem();
	newGFT = vfs_dbg_getGFTEntryCount();
	if(oldHeap != newHeap || oldGFT != newGFT) {
		test_caseFailed("oldHeap=%d, newHeap=%d, oldGFT=%d, newGFT=%d",oldHeap,newHeap,oldGFT,newGFT);
		return;
	}

	test_caseSucceded();
}

static s32 dummyReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
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
