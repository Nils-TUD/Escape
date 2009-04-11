/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <vfs.h>
#include <vfsnode.h>
#include <proc.h>
#include <kheap.h>
#include <video.h>
#include <string.h>
#include <errors.h>

#include "tvfs.h"
#include <test.h>

/* forward declarations */
static void test_vfs(void);
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
	char name[MAX_NAME_LEN + 1];
} __attribute__((packed)) sVFSDirEntryRead;

/* our test-module */
sTestModule tModVFS = {
	"VFS",
	&test_vfs
};

static void test_vfs(void) {
	test_vfs_createProcess();
	test_vfs_createService();
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
	if(oldHeap > newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}

static void test_vfs_createService(void) {
	u32 oldHeap,newHeap;
	s32 id,id2,id3;

	test_caseStart("Testing vfs_createService()");

	oldHeap = kheap_getFreeMem();

	id = vfs_createService(0,"test",0);
	if(!test_assertTrue(vfsn_isValidNodeNo(id))) return;
	id2 = vfs_createService(0,"test2",0);
	if(!test_assertTrue(vfsn_isValidNodeNo(id2))) return;
	if(!test_assertInt(vfs_createService(1,"test",0),ERR_SERVICE_EXISTS)) return;
	if(!test_assertInt(vfs_createService(1,"",0),ERR_INV_SERVICE_NAME)) return;
	if(!test_assertInt(vfs_createService(1,"abc.def",0),ERR_INV_SERVICE_NAME)) return;
	id3 = vfs_createService(1,"test3",0);
	if(!test_assertTrue(vfsn_isValidNodeNo(id3))) return;

	vfs_removeService(0,id);
	vfs_removeService(0,id2);
	vfs_removeService(1,id3);

	/* check mem-usage */
	newHeap = kheap_getFreeMem();
	if(oldHeap > newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}
