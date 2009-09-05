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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include "ext2.h"
#include "blockcache.h"
#include "inodecache.h"
#include "blockgroup.h"
#include "superblock.h"
#include "rw.h"

/**
 * Checks wether x is a power of y
 */
static bool ext2_isPowerOf(u32 x,u32 y);

bool ext2_init(sExt2 *e) {
	tFD fd;
	/* we have to try it multiple times in this case since the kernel loads ata and fs
	 * directly after another and we don't know who's ready first */
	/* TODO later the device for the root-partition should be chosen in the multiboot-parameters */
	do {
		fd = open("/drivers/hda1",IO_WRITE | IO_READ);
		if(fd < 0)
			yield();
	}
	while(fd < 0);

	e->ataFd = fd;
	if(!ext2_super_init(e)) {
		close(e->ataFd);
		return false;
	}

	/* init block-groups */
	if(!ext2_bg_init(e))
		return false;

	/* init caches */
	ext2_icache_init(e);
	ext2_bcache_init(e);
	return true;
}

void ext2_sync(sExt2 *e) {
	ext2_super_update(e);
	ext2_bg_update(e);
	/* flush inodes first, because they may create dirty blocks */
	ext2_icache_flush(e);
	ext2_bcache_flush(e);
}

u32 ext2_getBlockOfInode(sExt2 *e,tInodeNo inodeNo) {
	return (inodeNo - 1) / e->superBlock.inodesPerGroup;
}

u32 ext2_getGroupOfBlock(sExt2 *e,u32 block) {
	return block / e->superBlock.blocksPerGroup;
}

u32 ext2_getGroupOfInode(sExt2 *e,tInodeNo inodeNo) {
	return inodeNo / e->superBlock.inodesPerGroup;
}

u32 ext2_getBlockGroupCount(sExt2 *e) {
	u32 bpg = e->superBlock.blocksPerGroup;
	return (e->superBlock.blockCount + bpg - 1) / bpg;
}

bool ext2_bgHasBackups(sExt2 *e,u32 i) {
	/* if the sparse-feature is enabled, just the groups 0, 1 and powers of 3, 5 and 7 contain
	 * the backup */
	if(!(e->superBlock.featureRoCompat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return true;
	/* block-group 0 is handled manually */
	if(i == 1)
		return true;
	return ext2_isPowerOf(i,3) || ext2_isPowerOf(i,5) || ext2_isPowerOf(i,7);
}

void ext2_destroy(sExt2 *e) {
	free(e->groups);
	close(e->ataFd);
}

static bool ext2_isPowerOf(u32 x,u32 y) {
	while(x > 1) {
		if(x % y != 0)
			return false;
		x /= y;
	}
	return true;
}

#if DEBUGGING

void ext2_bg_prints(sExt2 *e) {
	u32 i,count = ext2_getBlockGroupCount(e);
	debugf("Blockgroups:\n");
	for(i = 0; i < count; i++) {
		debugf(" Block %d\n",i);
		ext2_bg_print(e,i,e->groups + i);
	}
}

#endif
