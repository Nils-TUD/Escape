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

#pragma once

#include <esc/common.h>

class Ext2FileSystem;

struct Ext2SuperBlock {
	/* the total number of inodes, both used and free, in the file system. */
	uint32_t inodeCount;
	/* the total number of blocks in the system including all used, free and reserved */
	uint32_t blockCount;
	/* number of blocks reserved for the super-user */
	uint32_t suResBlockCount;
	uint32_t freeBlockCount;
	uint32_t freeInodeCount;
	uint32_t firstDataBlock;
	/* blockSize = 1024 << logBlockSize; */
	uint32_t logBlockSize;
	/* if(positive)
	 *   fragmentSize = 1024 << logFragSize;
	 * else
	 *   fragmentSize = 1024 >> -logFragSize; */
	uint32_t logFragSize;
	uint32_t blocksPerGroup;
	uint32_t fragsPerGroup;
	uint32_t inodesPerGroup;
	uint32_t lastMountTime;
	uint32_t lastWritetime;
	/* number of mounts since last verification */
	uint16_t mountCount;
	/* max number of mounts until a full check has to be performed */
	uint16_t maxMountCount;
	/* should be EXT2_SUPER_MAGIC */
	uint16_t magic;
	/* EXT2_VALID_FS or EXT2_ERROR_FS */
	uint16_t state;
	/* on of EXT2_ERRORS_* */
	uint16_t errors;
	uint16_t minorRevLevel;
	uint32_t lastCheck;
	uint32_t checkInterval;
	uint32_t creatorOS;
	uint32_t revLevel;
	/* the default user-/group-id for reserved blocks */
	uint16_t defResUid;
	uint16_t defResGid;
	/* the index to the first inode useable for standard files. Always EXT2_REV0_FIRST_INO for
	 * revision 0. In revision 1 and later this value may be set to any value. */
	uint32_t firstInode;
	/* 16bit value indicating the size of the inode structure. In revision 0, this value is always
	 * 128 (EXT2_REV0_INODE_SIZE). In revision 1 and later, this value must be a perfect power of
	 * 2 and must be smaller or equal to the block size (1<<s_log_block_size). */
	uint16_t inodeSize;
	/* the block-group which hosts this super-block */
	uint16_t blockGroupNo;
	/* 32bit bitmask of compatible features */
	uint32_t featureCompat;
	/* 32bit bitmask of incompatible features */
	uint32_t featureInCompat;
	/* 32bit bitmask of “read-only” features. The file system implementation should mount as
	 * read-only if any of the indicated feature is unsupported. */
	uint32_t featureRoCompat;
	/* 128bit value used as the volume id. This should, as much as possible, be unique for
	 * each file system formatted. */
	uint8_t volumeUid[16];
	/* 16 bytes volume name, mostly unusued. A valid volume name would consist of only ISO-Latin-1
	 * characters and be 0 terminated. */
	char volumeName[16];
	/* 64 bytes directory path where the file system was last mounted. */
	char lastMountPath[64];
	/* 32bit value used by compression algorithms to determine the compression method(s) used. */
	uint32_t algoBitmap;
	/* 8-bit value representing the number of blocks the implementation should attempt to
	 * pre-allocate when creating a new regular file. */
	uint8_t preAllocBlocks;
	/* 8-bit value representing the number of blocks the implementation should attempt to
	 * pre-allocate when creating a new directory. */
	uint8_t preAllocDirBlocks;
	/* alignment */
	uint16_t : 16;
	/* 16-byte value containing the uuid of the journal superblock */
	uint8_t journalUid[16];
	/* 32-bit inode number of the journal file */
	uint32_t journalInodeNo;
	/* 32-bit device number of the journal file */
	uint32_t journalDev;
	/* 32-bit inode number, pointing to the first inode in the list of inodes to delete. */
	uint32_t lastOrphan;
	/* An array of 4 32bit values containing the seeds used for the hash algorithm for directory
	 * indexing. */
	uint32_t hashSeed[4];
	/* An 8bit value containing the default hash version used for directory indexing.*/
	uint8_t defHashVersion;
	/* padding */
	uint16_t : 16;
	uint8_t : 8;
	/* A 32bit value containing the default mount options for this file system. */
	uint32_t defMountOptions;
	/* A 32bit value indicating the block group ID of the first meta block group. */
	uint32_t firstMetaBg;
	/* UNUSED */
	uint8_t unused[760];
} A_PACKED;

class Ext2SBMng {
public:
	/**
	 * Inits the super-block; reads it from disk and reads the block-group-descriptor-table
	 *
	 * @param e the ext2-data
	 * @return 0 on success
	 */
	explicit Ext2SBMng(Ext2FileSystem *e);

	/**
	 * @return the superblock
	 */
	Ext2SuperBlock *get() {
		return &_superBlock;
	}
	const Ext2SuperBlock *get() const {
		return &_superBlock;
	}

	/**
	 * Marks the superblock as dirty
	 */
	void markDirty() {
		_sbDirty = true;
	}

	/**
	 * Updates the super-block, if it is dirty
	 *
	 * @param e the ext2-data
	 */
	void update();

private:
	bool _sbDirty;
	Ext2SuperBlock _superBlock;
	Ext2FileSystem *_fs;
};
