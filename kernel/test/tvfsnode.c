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
static bool test_vfsn_resolveRealPath(cstring a);
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

	if(!test_vfsn_resolvePathCpy("system:/..","system")) return;
	if(!test_vfsn_resolvePathCpy("system://../..","system")) return;
	if(!test_vfsn_resolvePathCpy("system://./.","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/","system")) return;
	if(!test_vfsn_resolvePathCpy("system://","system")) return;
	if(!test_vfsn_resolvePathCpy("system:///","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/..","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/.","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/./","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/./.","system")) return;
	if(!test_vfsn_resolvePathCpy("system://///processes/./././.","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/../processes/../processes/./","processes")) return;
	if(!test_vfsn_resolvePathCpy("system://..//..//..","system")) return;

	test_caseSucceded();
}

static bool test_vfsn_resolveRealPath(cstring a) {
	tVFSNodeNo no;
	return test_assertInt(vfsn_resolvePath(a,&no),ERR_REAL_PATH);
}

static bool test_vfsn_resolvePathCpy(cstring a,cstring b) {
	s32 err;
	tVFSNodeNo no;
	sVFSNode *node;
	if((err = vfsn_resolvePath(a,&no)) != 0) {
		test_caseFailed("Unable to resolve the path %s",a);
		return false;
	}

	node = vfsn_getNode(no);
	return test_assertStr(node->name,(string)b);
}

static void test_vfsn_getPath(void) {
	tVFSNodeNo no;

	test_caseStart("Testing vfsn_getPath()");

	vfsn_resolvePath("system:",&no);
	test_assertStr(vfsn_getPath(no),(string)"system:");

	vfsn_resolvePath("system:/processes",&no);
	test_assertStr(vfsn_getPath(no),(string)"system:/processes");

	vfsn_resolvePath("system:/processes/0",&no);
	test_assertStr(vfsn_getPath(no),(string)"system:/processes/0");

	test_caseSucceded();
}
