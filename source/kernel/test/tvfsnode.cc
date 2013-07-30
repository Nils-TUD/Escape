/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/vfs/openfile.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include "tvfsnode.h"
#include "testutils.h"
#include <esc/test.h>
#include <string.h>
#include <errno.h>

static void test_vfsn(void);
static void test_vfs_node_resolvePath(void);
static bool test_vfs_node_resolvePathCpy(const char *a,const char *b);
static void test_vfs_node_getPath(void);
static void test_vfs_node_file_refs(void);
static void test_vfs_node_dir_refs(void);

/* our test-module */
sTestModule tModVFSn = {
	"VFSNode",
	&test_vfsn
};

static void test_vfsn(void) {
	test_vfs_node_resolvePath();
	test_vfs_node_getPath();
	test_vfs_node_file_refs();
	test_vfs_node_dir_refs();
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
	inode_t no;
	sVFSNode *node;
	if((err = vfs_node_resolvePath(a,&no,NULL,VFS_READ)) != 0) {
		test_caseFailed("Unable to resolve the path %s",a);
		return false;
	}

	node = vfs_node_get(no);
	return test_assertStr(node->name,(char*)b);
}

static void test_vfs_node_getPath(void) {
	inode_t no;

	test_caseStart("Testing vfs_node_getPath()");

	vfs_node_resolvePath("/system",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system");

	vfs_node_resolvePath("/system/processes",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system/processes");

	vfs_node_resolvePath("/system/processes/0",&no,NULL,VFS_READ);
	test_assertStr(vfs_node_getPath(no),(char*)"/system/processes/0");

	test_caseSucceeded();
}

static void test_vfs_node_file_refs(void) {
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	OpenFile *f1,*f2;
	sVFSNode *n;
	char buffer[64] = "This is a test!";

	test_caseStart("Testing reference counting of file nodes");
	checkMemoryBefore(false);

	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar",&f1),0);
	n = f1->getNode();
	test_assertSSize(f1->writeFile(pid,buffer,strlen(buffer)),strlen(buffer));
	test_assertSize(n->refCount,2);
	f1->closeFile(pid);
	test_assertSize(n->refCount,1);

	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f2),0);
	test_assertSize(n->refCount,2);
	test_assertInt(VFS::unlink(pid,"/system/foobar"),0);
	test_assertSize(n->refCount,1);
	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f2),-ENOENT);
	memclear(buffer,sizeof(buffer));
	test_assertTrue(f2->readFile(pid,buffer,sizeof(buffer)) > 0);
	test_assertStr(buffer,"This is a test!");
	f2->closeFile(pid);

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_vfs_node_dir_refs(void) {
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	OpenFile *f1;
	sVFSNode *n,*f;
	inode_t nodeNo;
	bool valid;

	test_caseStart("Testing reference counting of dir nodes");
	checkMemoryBefore(false);

	test_assertInt(VFS::mkdir(pid,"/system/foobar"),0);
	test_assertInt(VFS::mkdir(pid,"/system/foobar/test"),0);
	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar/myfile1",&f1),0);
	f1->closeFile(pid);
	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar/myfile2",&f1),0);
	f1->closeFile(pid);

	test_assertInt(vfs_node_resolvePath("/system/foobar",&nodeNo,NULL,0),0);
	n = vfs_node_get(nodeNo);
	test_assertSize(n->refCount,1);
	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f1),0);
	test_assertSize(n->refCount,2);

	f = vfs_node_openDir(n,false,&valid);
	test_assertTrue(valid);

	test_assertStr(f->name,".");
	f = f->next;
	test_assertStr(f->name,"..");
	f = f->next;
	test_assertStr(f->name,"test");

	test_assertInt(VFS::unlink(pid,"/system/foobar"),0);
	test_assertSize(n->refCount,1);

	vfs_node_closeDir(n,false);
	f1->closeFile(pid);

	checkMemoryAfter(false);
	test_caseSucceeded();
}
