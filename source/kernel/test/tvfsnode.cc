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

static void test_vfsn();
static void test_vfs_node_resolvePath();
static bool test_vfs_node_resolvePathCpy(const char *a,const char *b);
static void test_vfs_node_getPath();
static void test_vfs_node_file_refs();
static void test_vfs_node_dir_refs();

/* our test-module */
sTestModule tModVFSn = {
	"VFSNode",
	&test_vfsn
};

static void test_vfsn() {
	test_vfs_node_resolvePath();
	test_vfs_node_getPath();
	test_vfs_node_file_refs();
	test_vfs_node_dir_refs();
}

static void test_vfs_node_resolvePath() {
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
	VFSNode *node;
	if((err = VFSNode::request(a,&node,NULL,VFS_READ)) != 0) {
		test_caseFailed("Unable to resolve the path %s",a);
		return false;
	}

	bool res = test_assertStr(node->getName(),(char*)b);
	VFSNode::release(node);
	return res;
}

static void test_vfs_node_getPath() {
	VFSNode *node;

	test_caseStart("Testing vfs_node_getPath()");

	VFSNode::request("/system",&node,NULL,VFS_READ);
	test_assertStr(node->getPath(),(char*)"/system");
	VFSNode::release(node);

	VFSNode::request("/system/processes",&node,NULL,VFS_READ);
	test_assertStr(node->getPath(),(char*)"/system/processes");
	VFSNode::release(node);

	VFSNode::request("/system/processes/0",&node,NULL,VFS_READ);
	test_assertStr(node->getPath(),(char*)"/system/processes/0");
	VFSNode::release(node);

	test_caseSucceeded();
}

static void test_vfs_node_file_refs() {
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	OpenFile *f1,*f2,*f3;
	VFSNode *n;
	char buffer[64] = "This is a test!";

	test_caseStart("Testing reference counting of file nodes");
	checkMemoryBefore(false);
	size_t nodesBefore = VFSNode::getNodeCount();

	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar",&f1),0);
	n = f1->getNode();
	test_assertSSize(f1->write(pid,buffer,strlen(buffer)),strlen(buffer));
	test_assertSize(n->getRefCount(),2);
	f1->close(pid);
	test_assertSize(n->getRefCount(),1);

	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f2),0);
	test_assertSize(n->getRefCount(),2);
	test_assertInt(VFS::unlink(pid,"/system/foobar"),0);
	test_assertSize(n->getRefCount(),1);
	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f3),-ENOENT);
	memclear(buffer,sizeof(buffer));
	test_assertTrue(f2->read(pid,buffer,sizeof(buffer)) > 0);
	test_assertStr(buffer,"This is a test!");
	f2->close(pid);

	test_assertSize(nodesBefore,VFSNode::getNodeCount());
	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_vfs_node_dir_refs() {
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	OpenFile *f1;
	VFSNode *n;
	bool valid;

	test_caseStart("Testing reference counting of dir nodes");
	checkMemoryBefore(false);
	size_t nodesBefore = VFSNode::getNodeCount();

	test_assertInt(VFS::mkdir(pid,"/system/foobar"),0);
	test_assertInt(VFS::mkdir(pid,"/system/foobar/test"),0);
	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar/myfile1",&f1),0);
	f1->close(pid);
	test_assertInt(VFS::openPath(pid,VFS_WRITE | VFS_CREATE,"/system/foobar/myfile2",&f1),0);
	f1->close(pid);

	test_assertInt(VFSNode::request("/system/foobar",&n,NULL,0),0);
	test_assertSize(n->getRefCount(),2);
	VFSNode::release(n);
	test_assertSize(n->getRefCount(),1);
	test_assertInt(VFS::openPath(pid,VFS_READ,"/system/foobar",&f1),0);
	test_assertSize(n->getRefCount(),2);

	const VFSNode *f = n->openDir(false,&valid);
	test_assertTrue(valid);

	test_assertStr(f->getName(),".");
	f = f->next;
	test_assertStr(f->getName(),"..");
	f = f->next;
	test_assertStr(f->getName(),"test");

	test_assertInt(VFS::rmdir(pid,"/system/foobar/test"),0);
	test_assertInt(VFS::unlink(pid,"/system/foobar/myfile1"),0);
	test_assertInt(VFS::unlink(pid,"/system/foobar/myfile2"),0);
	test_assertInt(VFS::rmdir(pid,"/system/foobar"),0);
	test_assertSize(n->getRefCount(),1);

	n->closeDir(false);
	f1->close(pid);

	test_assertSize(nodesBefore,VFSNode::getNodeCount());
	checkMemoryAfter(false);
	test_caseSucceeded();
}
