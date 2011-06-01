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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include "tvfsnode.h"
#include <esc/test.h>
#include <string.h>
#include <errors.h>

static void test_vfsn(void);
static void test_vfs_node_resolvePath(void);
static bool test_vfs_node_resolvePathCpy(const char *a,const char *b);
static void test_vfs_node_getPath(void);

/* our test-module */
sTestModule tModVFSn = {
	"VFSNode",
	&test_vfsn
};

static void test_vfsn(void) {
	test_vfs_node_resolvePath();
	test_vfs_node_getPath();
}

static void test_vfs_node_resolvePath(void) {
	test_caseStart("Testing vfs_node_resolvePath()");

	if(!test_vfs_node_resolvePathCpy("/system/..","")) return;
	if(!test_vfs_node_resolvePathCpy("/system//../..","")) return;
	if(!test_vfs_node_resolvePathCpy("/system//./.","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system/","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system//","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system///","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system/processes/..","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system/processes/.","processes")) return;
	if(!test_vfs_node_resolvePathCpy("/system/processes/./","processes")) return;
	if(!test_vfs_node_resolvePathCpy("/system/./.","system")) return;
	if(!test_vfs_node_resolvePathCpy("/system/////processes/./././.","processes")) return;
	if(!test_vfs_node_resolvePathCpy("/system/./processes/../processes/./","processes")) return;
	if(!test_vfs_node_resolvePathCpy("/system//..//..//..","")) return;

	test_caseSucceeded();
}

static bool test_vfs_node_resolvePathCpy(const char *a,const char *b) {
	int err;
	tInodeNo no;
	sVFSNode *node;
	if((err = vfs_node_resolvePath(a,&no,NULL,VFS_READ)) != 0) {
		test_caseFailed("Unable to resolve the path %s",a);
		return false;
	}

	node = vfs_node_get(no);
	return test_assertStr(node->name,(char*)b);
}

static void test_vfs_node_getPath(void) {
	tInodeNo no;

	test_caseStart("Testing vfs_node_getPath()");

	vfs_node_resolvePath("/system",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system");

	vfs_node_resolvePath("/system/processes",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system/processes");

	vfs_node_resolvePath("/system/processes/0",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system/processes/0");

	test_caseSucceeded();
}
