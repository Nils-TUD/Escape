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
#include <esc/thread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ext2.h"
#include "rw.h"
#include "../blockcache.h"
#include "inodecache.h"
#include "superblock.h"

bool ext2_super_init(sExt2 *e) {
	e->sbDirty = false;
	/* read sector-based because we need the super-block to be able to read block-based */
	if(!ext2_rw_readSectors(e,&(e->superBlock),EXT2_SUPERBLOCK_SECNO,2)) {
		printe("Unable to read super-block");
		return false;
	}

	/* check magic-number */
	if(e->superBlock.magic != EXT2_SUPER_MAGIC) {
		printe("Invalid super-magic: Got %x, expected %x",e->superBlock.magic,EXT2_SUPER_MAGIC);
		return false;
	}

	/* check features */
	/* TODO mount readonly if an unsupported feature is present in featureRoCompat */
	if(e->superBlock.featureInCompat || e->superBlock.featureRoCompat) {
		printe("Unable to use filesystem: Incompatible features");
		return false;
	}
	return true;
}

void ext2_super_update(sExt2 *e) {
	u32 i,count,bno;
	assert(lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	if(!e->sbDirty)
		goto done;

	/* update main superblock; sector-based because we want to update the super-block without
	 * anything else (this would happen if the superblock is in the block 0 together with
	 * the bootrecord) */
	if(!ext2_rw_writeSectors(e,&(e->superBlock),EXT2_SUPERBLOCK_SECNO,2)) {
		printe("Unable to update superblock in blockgroup 0");
		goto done;
	}

	/* update superblock backups */
	bno = e->superBlock.blocksPerGroup;
	count = ext2_getBlockGroupCount(e);
	for(i = 1; i < count; i++) {
		if(ext2_bgHasBackups(e,i)) {
			if(!ext2_rw_writeBlocks(e,&(e->superBlock),bno,1)) {
				printe("Unable to update superblock in blockgroup %d",i);
				goto done;
			}
		}
		bno += e->superBlock.blocksPerGroup;
	}

	/* now we're in sync */
	e->sbDirty = false;
done:
	assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
}
