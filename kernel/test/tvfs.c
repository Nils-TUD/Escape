/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/vfs.h"
#include "../pub/video.h"
#include "../pub/string.h"
#include "../pub/proc.h"
#include "../pub/kheap.h"

#include "tvfs.h"
#include "test.h"

/* forward declarations */
static void test_vfs(void);
static void test_vfs_readFileSystem(void);
static void test_vfs_readFileProcess0(void);
static void test_vfs_createProcessNode(void);
static void test_vfs_cleanPath(void);
static bool test_vfs_cleanPathCpy(cstring a,cstring b);

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
	test_vfs_cleanPath();
	test_vfs_readFileSystem();
	test_vfs_readFileProcess0();
	test_vfs_createProcessNode();
}

static void test_vfs_readFileSystem(void) {
	tVFSNodeNo nodeNo,procNode,servNode;
	tFD fd;
	s32 res;
	sVFSNodePub node;
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for /system");

	/* resolve path */
	if(vfs_resolvePath("/system",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path '/system'!");
		return;
	}

	/* open */
	if(vfs_openFile(GFT_READ,nodeNo,&fd) < 0) {
		test_caseFailed("Unable to open '/system'!");
		return;
	}

	/* read "processes" */
	if((res = vfs_readFile(fd,(u8*)&node,sizeof(sVFSNodePub))) != sizeof(sVFSNodePub)) {
		vfs_closeFile(fd);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sVFSNodePub),res);
		return;
	}

	/* check data */
	vfs_resolvePath("/system/processes",&procNode);
	if(strcmp((cstring)node.name,"processes") != 0) {
		vfs_closeFile(fd);
		test_caseFailed("Node-name='%s', expected 'system'",node.name);
		return;
	}
	if(node.nodeNo != procNode) {
		vfs_closeFile(fd);
		test_caseFailed("nodeNo=%d, expected %d",node.nodeNo);
		return;
	}

	/* read "services" */
	if((res = vfs_readFile(fd,(u8*)&node,sizeof(sVFSNodePub))) != sizeof(sVFSNodePub)) {
		vfs_closeFile(fd);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sVFSNodePub),res);
		return;
	}

	/* check data */
	vfs_resolvePath("/system/services",&servNode);
	if(strcmp((cstring)node.name,"services") != 0) {
		vfs_closeFile(fd);
		test_caseFailed("Node-name='%s', expected 'system'",node.name);
		return;
	}
	if(node.nodeNo != servNode) {
		vfs_closeFile(fd);
		test_caseFailed("nodeNo=%d, expected %d",node.nodeNo);
		return;
	}

	/* close */
	vfs_closeFile(fd);

	newHeap = kheap_getFreeMem();
	newGFT = vfs_getGFTEntryCount();
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
	u32 oldHeap,oldGFT,newHeap,newGFT;

	oldHeap = kheap_getFreeMem();
	oldGFT = vfs_getGFTEntryCount();

	test_caseStart("Testing vfs_readFile() for /system/processes/0");

	/* resolve path */
	if(vfs_resolvePath("/system/processes/0",&nodeNo) < 0) {
		test_caseFailed("Unable to resolve path '/system/processes/0'!");
		return;
	}

	/* open */
	if(vfs_openFile(GFT_READ,nodeNo,&fd) < 0) {
		test_caseFailed("Unable to open '/system/processes/0'!");
		return;
	}

	/* read */
	if((res = vfs_readFile(fd,(u8*)&proc,sizeof(sProcPub))) != sizeof(sProcPub)) {
		vfs_closeFile(fd);
		test_caseFailed("Unable to read with fd=%d! Expected %d bytes, read %d",
				fd,sizeof(sProcPub),res);
		return;
	}

	/* check data */
	if(!test_assertInt(proc.textPages,p0->textPages)) {vfs_closeFile(fd); return;}
	if(!test_assertInt(proc.dataPages,p0->dataPages)) {vfs_closeFile(fd); return;}
	if(!test_assertInt(proc.stackPages,p0->stackPages)) {vfs_closeFile(fd); return;}
	if(!test_assertInt(proc.pid,p0->pid)) {vfs_closeFile(fd); return;}
	if(!test_assertInt(proc.parentPid,p0->parentPid)) {vfs_closeFile(fd); return;}
	if(!test_assertInt(proc.state,p0->state)) {vfs_closeFile(fd); return;}

	/* close */
	vfs_closeFile(fd);

	newHeap = kheap_getFreeMem();
	newGFT = vfs_getGFTEntryCount();
	if(oldHeap != newHeap || oldGFT != newGFT) {
		test_caseFailed("oldHeap=%d, newHeap=%d, oldGFT=%d, newGFT=%d",oldHeap,newHeap,oldGFT,newGFT);
		return;
	}

	test_caseSucceded();
}

static s32 dummyReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return 0;
}

static void test_vfs_createProcessNode(void) {
	u32 oldHeap,newHeap;

	test_caseStart("Testing vfs_createProcessNode()");

	oldHeap = kheap_getFreeMem();

	/* create */
	if(!vfs_createProcessNode(123,&dummyReadHandler)) {
		test_caseFailed("Unable to create node with process id 123");
		return;
	}
	/* try another create with same pid */
	if(vfs_createProcessNode(123,&dummyReadHandler)) {
		test_caseFailed("Created another node with pid 123");
		return;
	}
	/* remove */
	vfs_removeProcessNode(123);

	/* check mem-usage */
	newHeap = kheap_getFreeMem();
	if(oldHeap != newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}

static void test_vfs_cleanPath(void) {
	test_caseStart("Testing vfs_cleanPath()");

	if(!test_vfs_cleanPathCpy("/","/")) return;
	if(!test_vfs_cleanPathCpy("//","/")) return;
	if(!test_vfs_cleanPathCpy("///","/")) return;
	if(!test_vfs_cleanPathCpy("","")) return;
	if(!test_vfs_cleanPathCpy("/abc","/abc")) return;
	if(!test_vfs_cleanPathCpy("/abc/","/abc")) return;
	if(!test_vfs_cleanPathCpy("//abc//","/abc")) return;
	if(!test_vfs_cleanPathCpy("/..","/")) return;
	if(!test_vfs_cleanPathCpy("/../..","/")) return;
	if(!test_vfs_cleanPathCpy("/./.","/")) return;
	if(!test_vfs_cleanPathCpy("/system/..","/")) return;
	if(!test_vfs_cleanPathCpy("/system/.","/system")) return;
	if(!test_vfs_cleanPathCpy("/system/./","/system")) return;
	if(!test_vfs_cleanPathCpy("/system/./.","/system")) return;
	if(!test_vfs_cleanPathCpy("/////system/./././.","/system")) return;
	if(!test_vfs_cleanPathCpy("/../system/../system/./","/system")) return;
	if(!test_vfs_cleanPathCpy(".....",".....")) return;
	if(!test_vfs_cleanPathCpy("..","")) return;
	if(!test_vfs_cleanPathCpy(".","")) return;
	if(!test_vfs_cleanPathCpy("//..//..//..","/")) return;
	if(!test_vfs_cleanPathCpy("..//..//..","")) return;
	if(!test_vfs_cleanPathCpy("../..","")) return;
	if(!test_vfs_cleanPathCpy("./.","")) return;
	if(!test_vfs_cleanPathCpy("system/..","")) return;
	if(!test_vfs_cleanPathCpy("system/.","system")) return;
	if(!test_vfs_cleanPathCpy("system/./","system")) return;
	if(!test_vfs_cleanPathCpy("system/./.","system")) return;
	if(!test_vfs_cleanPathCpy("../system/../system/./","system")) return;
	if(!test_vfs_cleanPathCpy(".////////","")) return;

	test_caseSucceded();
}

static bool test_vfs_cleanPathCpy(cstring a,cstring b) {
	static s8 c[100];
	strncpy(c,b,99);
	return test_assertStr(vfs_cleanPath((string)a),c);
}
