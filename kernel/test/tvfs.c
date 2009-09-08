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
#include <task/proc.h>
#include <mem/kheap.h>
#include <video.h>
#include <string.h>
#include <errors.h>

#include "tvfs.h"
#include <test.h>

/* forward declarations */
static void test_vfs(void);
static void test_vfs_createService(void);

/* public vfs-node for test-purposes */
typedef struct {
	tInodeNo nodeNo;
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
