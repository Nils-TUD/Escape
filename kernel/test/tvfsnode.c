/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "tvfsnode.h"
#include <test.h>
#include <string.h>

static void test_vfsn(void);
static void test_vfsn_resolvePath(void);
static bool test_vfsn_resolvePathCpy(cstring a,cstring b);
static void test_vfsn_getPath(void);

/* our test-module */
sTestModule tModVFSn = {
	"VFSNode",
	&test_vfsn
};

static void test_vfsn(void) {
	test_vfsn_resolvePath();
	test_vfsn_getPath();
}

static void test_vfsn_resolvePath(void) {
	test_caseStart("Testing vfsn_resolvePath()");

	if(!test_vfsn_resolvePathCpy("/","")) return;
	if(!test_vfsn_resolvePathCpy("//","")) return;
	if(!test_vfsn_resolvePathCpy("///","")) return;
	if(!test_vfsn_resolvePathCpy("/..","")) return;
	if(!test_vfsn_resolvePathCpy("/../..","")) return;
	if(!test_vfsn_resolvePathCpy("/./.","")) return;
	if(!test_vfsn_resolvePathCpy("/system/..","")) return;
	if(!test_vfsn_resolvePathCpy("/system/.","system")) return;
	if(!test_vfsn_resolvePathCpy("/system/./","system")) return;
	if(!test_vfsn_resolvePathCpy("/system/./.","system")) return;
	if(!test_vfsn_resolvePathCpy("/////system/./././.","system")) return;
	if(!test_vfsn_resolvePathCpy("/../system/../system/./","system")) return;
	if(!test_vfsn_resolvePathCpy("//..//..//..","")) return;
	/* TODO test as soon as relative paths are possible
	if(!test_vfsn_resolvePathCpy("system/.","system")) return;
	if(!test_vfsn_resolvePathCpy("system/./","system")) return;
	if(!test_vfsn_resolvePathCpy("system/./.","system")) return;
	if(!test_vfsn_resolvePathCpy("../system/../system/./","system")) return; */

	test_caseSucceded();
}

static bool test_vfsn_resolvePathCpy(cstring a,cstring b) {
	static s8 c[100];
	s32 err;
	tVFSNodeNo no;
	sVFSNode *node;
	strncpy(c,b,99);
	if((err = vfsn_resolvePath(a,&no)) != 0)
		return err;

	node = vfsn_getNode(no);
	return test_assertStr(node->name,c);
}

static void test_vfsn_getPath(void) {
	tVFSNodeNo no;
	sVFSNode *node;

	test_caseStart("Testing vfsn_getPath()");

	vfsn_resolvePath("/",&no);
	node = vfsn_getNode(no);
	test_assertStr(vfsn_getPath(node),(string)"/");

	vfsn_resolvePath("/system",&no);
	node = vfsn_getNode(no);
	test_assertStr(vfsn_getPath(node),(string)"/system");

	vfsn_resolvePath("/system/processes",&no);
	node = vfsn_getNode(no);
	test_assertStr(vfsn_getPath(node),(string)"/system/processes");

	vfsn_resolvePath("/system/processes/0",&no);
	node = vfsn_getNode(no);
	test_assertStr(vfsn_getPath(node),(string)"/system/processes/0");

	test_caseSucceded();
}
