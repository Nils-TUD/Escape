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

#include <fs/blockcache.h>
#include <fs/common.h>
#include <fs/filesystem.h>
#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/endian.h>

#include "bgmng.h"
#include "dir.h"
#include "inodecache.h"
#include "sbmng.h"

static const size_t DISK_SECTOR_SIZE		= 512;
static const size_t EXT2_ICACHE_SIZE		= 64;
static const size_t EXT2_BCACHE_SIZE		= 2048;

static const uint EXT2_SUPERBLOCK_LOCK		= 0xF7180002;

class Ext2FileSystem : public fs::FileSystem<fs::OpenFile> {
public:
	class Ext2BlockCache : public fs::BlockCache {
	public:
		explicit Ext2BlockCache(Ext2FileSystem *fs)
			: BlockCache(fs->fd,EXT2_BCACHE_SIZE,fs->blockSize()), _fs(fs) {
		}

		bool readBlocks(void *buffer,block_t start,size_t blockCount) override;
		bool writeBlocks(const void *buffer,size_t start,size_t blockCount) override;

	private:
		Ext2FileSystem *_fs;
	};

	explicit Ext2FileSystem(const char *device);
	virtual ~Ext2FileSystem();

	ino_t open(fs::User *u,const char *path,uint flags,mode_t mode,int fd,
		fs::OpenFile **file) override;
	void close(fs::OpenFile *file) override;
	ino_t find(fs::User *u,fs::OpenFile *dir,const char *name);
	int stat(fs::OpenFile *file,struct ::stat *info) override;
	ssize_t read(fs::OpenFile *file,void *buffer,off_t offset,size_t size) override;
	ssize_t write(fs::OpenFile *file,const void *buffer,off_t offset,size_t size) override;
	int link(fs::User *u,fs::OpenFile *dst,fs::OpenFile *dir,const char *name) override;
	int linkIno(fs::User *u,ino_t dst,fs::OpenFile *dir,const char *name,bool isdir);
	int doUnlink(fs::User *u,fs::OpenFile *dir,const char *name,bool isdir);
	int unlink(fs::User *u,fs::OpenFile *dir,const char *name) override;
	int mkdir(fs::User *u,fs::OpenFile *dir,const char *name,mode_t mode) override;
	int rmdir(fs::User *u,fs::OpenFile *dir,const char *name) override;
	int rename(fs::User *u,fs::OpenFile *oldDir,const char *oldName,fs::OpenFile *newDir,
		const char *newName) override;
	int chmod(fs::User *u,fs::OpenFile *file,mode_t mode) override;
	int chown(fs::User *u,fs::OpenFile *file,uid_t uid,gid_t gid) override;
	int utime(fs::User *u,fs::OpenFile *file,const struct utimbuf *utimes) override;
	int truncate(fs::User *u,fs::OpenFile *file,off_t length) override;
	void sync() override;
	void print(FILE *f) override;

	/**
	 * Checks whether the given user has the permission <perms> for <cnode>. <perms> should
	 * contain the bits from each entity, i.e. for example S_IRUSR | S_IRGRP |
	 * S_IROTH when reading should be performed.
	 *
	 * @param cnode the cached inode
	 * @param u the user
	 * @param perms the required permissions
	 * @return 0 if the user has permission, the error-code otherwise
	 */
	int hasPermission(Ext2CInode *cnode,fs::User *u,uint perms);

	/**
	 * Checks whether the given user has the permission to remove <file> from <dir>.
	 *
	 * @param dir the directory to remove the file from
	 * @param file the file to remove
	 * @param u the user
	 * @return 0 if the user has permission, the error-code otherwise
	 */
	int canRemove(Ext2CInode *dir,Ext2CInode *file,fs::User *u);

	/**
	 * @return the block size of the filesystem
	 */
	size_t blockSize() const {
		return DISK_SECTOR_SIZE << (le32tocpu(sb.get()->logBlockSize) + 1);
	}

	/**
	 * Translates from blocks to sectors.
	 *
	 * @return the number of sectors
	 */
	size_t blocksToSecs(ulong n) const {
		return n << (le32tocpu(sb.get()->logBlockSize) + 1);
	}

	/**
	 * Translates from sectors to blocks.
	 *
	 * @return the number of blocks
	 */
	size_t secsToBlocks(ulong n) const {
		return n >> (le32tocpu(sb.get()->logBlockSize) + 1);
	}

	/**
	 * Translates from bytes to blocks.
	 *
	 * @return the number of blocks
	 */
	size_t bytesToBlocks(size_t bytes) const {
		return (bytes + blockSize() - 1) / blockSize();
	}

	/**
	 * Determines the block of the given inode
	 *
	 * @param e the ext2-data
	 * @param inodeNo the inode-number
	 * @return the block-number
	 */
	block_t getBlockOfInode(ino_t inodeNo) {
		return (inodeNo - 1) / le32tocpu(sb.get()->inodesPerGroup);
	}

	/**
	 * Determines the block-group of the given block
	 *
	 * @param e the ext2-data
	 * @param block the block-number
	 * @return the block-group-number
	 */
	block_t getGroupOfBlock(block_t block) {
		return block / le32tocpu(sb.get()->blocksPerGroup);
	}

	/**
	 * Determines the block-group of the given inode
	 *
	 * @param e the ext2-data
	 * @param inodeNo the inode-number
	 * @return the block-group-number
	 */
	block_t getGroupOfInode(ino_t inodeNo) {
		return inodeNo / le32tocpu(sb.get()->inodesPerGroup);
	}

	/**
	 * @param e the ext2-data
	 * @return the number of block-groups
	 */
	size_t getBlockGroupCount() {
		size_t bpg = le32tocpu(sb.get()->blocksPerGroup);
		return (le32tocpu(sb.get()->blockCount) + bpg - 1) / bpg;
	}

	/**
	 * @return true if y is a power of y
	 */
	bool isPowerOf(uint x,uint y) {
		while(x > 1) {
			if(x % y != 0)
				return false;
			x /= y;
		}
		return true;
	}

	/**
	 * Determines if the given block-group should contain a backup of the super-block
	 * and block-group-descriptor-table
	 *
	 * @param e the ext2-data
	 * @param i the block-group-number
	 * @return true if so
	 */
	bool bgHasBackups(block_t i);

#if DEBUGGING
	/**
	 * Prints all block-groups
	 *
	 * @param e the ext2-data
	 */
	void printBlockGroups();
#endif

	/* the fd for the device */
	int fd;

	/* superblock and blockgroups of that ext2-fs */
	Ext2SBMng sb;
	Ext2BGMng bgs;

	/* caches */
	Ext2INodeCache inodeCache;
	Ext2BlockCache blockCache;
};
