/**
 * @version		$Id: main.c 54 2008-11-15 15:07:46Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/video.h"
#include "../h/string.h"

#include "tvfs.h"
#include "test.h"

/* forward declarations */
static void test_vfs(void);
static void test_vfs_cleanPath(void);
static bool test_vfs_cleanPathCpy(cstring a,cstring b);

/* our test-module */
tTestModule tModVFS = {
	"VFS",
	&test_vfs
};

static void test_vfs(void) {
	test_vfs_cleanPath();
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
