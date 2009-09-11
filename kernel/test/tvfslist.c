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

#include <common.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/listeners.h>
#include <test.h>

static void test_vfslist(void);
static void test_vfslist_t1(void);
static void test_vfslist_t2(void);
static void test_vfslist_l1(sVFSNode *node,u32 event);
static void test_vfslist_l2(sVFSNode *node,u32 event);

/* our test-module */
sTestModule tModVFSList = {
	"VFS-Listeners",
	&test_vfslist
};

static u32 crtCount = 0;
static u32 modCount = 0;
static u32 delCount = 0;
static sVFSNode *sysNode;

static void test_vfslist(void) {
	test_vfslist_t1();
	test_vfslist_t2();
}

static void test_vfslist_t1(void) {
	tInodeNo nodeNo;
	test_caseStart("Testing one listener for all events");

	test_assertInt(vfsn_resolvePath("/system",&nodeNo,NULL,VFS_READ),0);
	sysNode = vfsn_getNode(nodeNo);
	test_assertInt(vfsl_add(sysNode,VFS_EV_CREATE,test_vfslist_l1),0);
	test_assertInt(vfsl_add(sysNode,VFS_EV_MODIFY,test_vfslist_l1),0);
	test_assertInt(vfsl_add(sysNode,VFS_EV_DELETE,test_vfslist_l1),0);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	vfsl_notify(sysNode->firstChild,VFS_EV_MODIFY);
	vfsl_notify(sysNode->firstChild,VFS_EV_DELETE);
	test_assertUInt(crtCount,1);
	test_assertUInt(modCount,1);
	test_assertUInt(delCount,1);

	vfsl_remove(sysNode,VFS_EV_CREATE,test_vfslist_l1);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	vfsl_notify(sysNode->firstChild,VFS_EV_MODIFY);
	vfsl_notify(sysNode->firstChild,VFS_EV_DELETE);
	test_assertUInt(crtCount,1);
	test_assertUInt(modCount,2);
	test_assertUInt(delCount,2);

	vfsl_remove(sysNode,VFS_EV_MODIFY,test_vfslist_l1);
	vfsl_remove(sysNode,VFS_EV_DELETE,test_vfslist_l1);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	vfsl_notify(sysNode->firstChild,VFS_EV_MODIFY);
	vfsl_notify(sysNode->firstChild,VFS_EV_DELETE);
	test_assertUInt(crtCount,1);
	test_assertUInt(modCount,2);
	test_assertUInt(delCount,2);

	test_caseSucceded();
}

static void test_vfslist_t2(void) {
	tInodeNo nodeNo;
	test_caseStart("Testing multiple listeners");

	/* first reset counts to know their values */
	crtCount = 0;
	modCount = 0;
	delCount = 0;

	test_assertInt(vfsn_resolvePath("/system",&nodeNo,NULL,VFS_READ),0);
	sysNode = vfsn_getNode(nodeNo);

	test_assertInt(vfsl_add(sysNode,VFS_EV_CREATE | VFS_EV_MODIFY,test_vfslist_l1),0);
	test_assertInt(vfsl_add(sysNode,VFS_EV_CREATE,test_vfslist_l2),0);
	test_assertInt(vfsl_add(sysNode,VFS_EV_CREATE,test_vfslist_l2),0);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	vfsl_notify(sysNode->firstChild,VFS_EV_MODIFY);
	test_assertUInt(crtCount,3);
	test_assertUInt(modCount,1);
	test_assertUInt(delCount,0);

	vfsl_remove(sysNode,VFS_EV_CREATE,test_vfslist_l2);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	test_assertUInt(crtCount,4);
	test_assertUInt(modCount,1);
	test_assertUInt(delCount,0);

	vfsl_remove(sysNode,VFS_EV_CREATE,test_vfslist_l1);

	vfsl_notify(sysNode->firstChild,VFS_EV_CREATE);
	vfsl_notify(sysNode->firstChild,VFS_EV_MODIFY);
	test_assertUInt(crtCount,4);
	test_assertUInt(modCount,2);
	test_assertUInt(delCount,0);

	vfsl_remove(sysNode,VFS_EV_MODIFY,test_vfslist_l1);

	test_caseSucceded();
}

static void test_vfslist_l1(sVFSNode *node,u32 event) {
	if(event == VFS_EV_CREATE)
		crtCount++;
	else if(event == VFS_EV_MODIFY)
		modCount++;
	else if(event == VFS_EV_DELETE)
		delCount++;
	test_assertPtr(node,sysNode->firstChild);
}

static void test_vfslist_l2(sVFSNode *node,u32 event) {
	if(event == VFS_EV_CREATE)
		crtCount++;
	else if(event == VFS_EV_MODIFY)
		modCount++;
	else if(event == VFS_EV_DELETE)
		delCount++;
	test_assertPtr(node,sysNode->firstChild);
}
