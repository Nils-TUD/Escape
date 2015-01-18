/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <sys/io.h>
#include <sys/thread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext2.h"
#include "inodecache.h"
#include "rw.h"
#include "sbmng.h"

Ext2SBMng::Ext2SBMng(Ext2FileSystem *fs) : _sbDirty(false), _fs(fs) {
	int res;
	/* read sector-based because we need the super-block to be able to read block-based */
	if((res = Ext2RW::readSectors(fs,&_superBlock,EXT2_SUPERBLOCK_SECNO,2)) < 0)
		VTHROWE("Unable to read super-block",res);

	/* check magic-number */
	if(le16tocpu(_superBlock.magic) != EXT2_SUPER_MAGIC) {
		VTHROW("Invalid super-magic: Got " << le16tocpu(_superBlock.magic)
										   << ", expected " << EXT2_SUPER_MAGIC);
	}

	/* check features */
	/* TODO mount readonly if an unsupported feature is present in featureRoCompat */
	if(le32tocpu(_superBlock.featureInCompat) || le32tocpu(_superBlock.featureRoCompat))
		VTHROWE("Unable to use filesystem: Incompatible features",-ENOTSUP);
}

void Ext2SBMng::update() {
	size_t i,count;
	block_t bno;
	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	if(!_sbDirty)
		goto done;

	/* update main superblock; sector-based because we want to update the super-block without
	 * anything else (this would happen if the superblock is in the block 0 together with
	 * the bootrecord) */
	if(Ext2RW::writeSectors(_fs,&_superBlock,EXT2_SUPERBLOCK_SECNO,2) != 0) {
		printe("Unable to update superblock in blockgroup 0");
		goto done;
	}

	/* update superblock backups */
	bno = le32tocpu(_superBlock.blocksPerGroup);
	count = _fs->getBlockGroupCount();
	for(i = 1; i < count; i++) {
		if(_fs->bgHasBackups(i)) {
			if(Ext2RW::writeBlocks(_fs,&_superBlock,bno,1) != 0) {
				printe("Unable to update superblock in blockgroup %d",i);
				goto done;
			}
		}
		bno += le32tocpu(_superBlock.blocksPerGroup);
	}

	/* now we're in sync */
	_sbDirty = false;
done:
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
}
