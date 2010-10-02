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
#include <sys/task/proc.h>
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <string.h>
#include <errors.h>

#include "tvfs.h"
#include <esc/test.h>

/* forward declarations */
static void test_vfs(void);
static void test_vfs_createDriver(void);

/* public vfs-node for test-purposes */
typedef struct {
	tInodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} A_PACKED sVFSDirEntry;
typedef struct {
	sVFSDirEntry header;
	char name[MAX_NAME_LEN + 1];
} A_PACKED sVFSDirEntryRead;

/* our test-module */
sTestModule tModVFS = {
	"VFS",
	&test_vfs
};

static void test_vfs(void) {
	test_vfs_createDriver();
}

static void test_vfs_createDriver(void) {
	u32 oldHeap,newHeap;
	s32 id,id2,id3;

	test_caseStart("Testing vfs_createDriver()");

	oldHeap = kheap_getFreeMem();

	id = vfs_createDriver(0,"test",0);
	if(!test_assertTrue(vfs_node_isValid(id))) return;
	id2 = vfs_createDriver(0,"test2",0);
	if(!test_assertTrue(vfs_node_isValid(id2))) return;
	if(!test_assertInt(vfs_createDriver(1,"test",0),ERR_DRIVER_EXISTS)) return;
	if(!test_assertInt(vfs_createDriver(1,"",0),ERR_INV_DRIVER_NAME)) return;
	if(!test_assertInt(vfs_createDriver(1,"abc.def",0),ERR_INV_DRIVER_NAME)) return;
	id3 = vfs_createDriver(1,"test3",0);
	if(!test_assertTrue(vfs_node_isValid(id3))) return;

	vfs_node_destroy(vfs_node_get(id));
	vfs_node_destroy(vfs_node_get(id2));
	vfs_node_destroy(vfs_node_get(id3));

	/* check mem-usage */
	newHeap = kheap_getFreeMem();
	if(oldHeap > newHeap) {
		test_caseFailed("oldHeap=%d, newHeap=%d",oldHeap,newHeap);
		return;
	}

	test_caseSucceded();
}
