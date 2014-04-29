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
#include <esc/sllist.h>
#include <esc/fsinterface.h>
#include <esc/endian.h>
#include <fs/common.h>
#include <fs/blockcache.h>
#include <fs/filesystem.h>

#include "bgmng.h"
#include "sbmng.h"
#include "inodecache.h"
#include "dir.h"

#define DISK_SECTOR_SIZE					512
#define EXT2_SUPERBLOCK_LOCK				0xF7180002

/* first sector of the super-block in the first block-group */
#define EXT2_SUPERBLOCK_SECNO				2
/* first block-number of the block-group-descriptor-table in other block-groups than the first */
#define EXT2_BLOGRPTBL_BNO					2

/* number of direct blocks in the inode */
#define EXT2_DIRBLOCK_COUNT					12

#define EXT2_ICACHE_SIZE					64
#define EXT2_BCACHE_SIZE					512

/* padding for directory-entries */
#define	EXT2_DIRENTRY_PAD					4

/* magic number */
#define EXT2_SUPER_MAGIC					0xEF53

/* states */
#define EXT2_VALID_FS						1
#define EXT2_ERROR_FS						2

/* how to handle errors */
/* continue as if nothing happened */
#define EXT2_ERRORS_CONTINUE				1
/* remount read-only */
#define EXT2_ERRORS_RO						2
/* cause a kernel panic */
#define EXT2_ERRORS_PANIC					3

/* creator os's */
#define EXT2_OS_LINUX						0
#define EXT2_OS_HURD						1
#define EXT2_OS_MASIX						2
#define EXT2_OS_FREEBSD						3
#define EXT2_OS_LITES						4

/* revisions */
#define EXT2_GOOD_OLD_REV					0
/*  Revision 1 with variable inode sizes, extended attributes, etc. */
#define EXT2_DYNAMIC_REV					1

/* first inode-number for standard-files in revision 0 */
#define EXT2_REV0_FIRST_INO					11
/* inode-size in rev0 */
#define EXT2_REV0_INODE_SIZE				128

/* compatible features */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO		0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020

/* incompatible features */
#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG		0x0010

/* read-only features */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004

/* compression algorithms */
#define EXT2_LZV1_ALG						0x0001
#define EXT2_LZRW3A_ALG						0x0002
#define EXT2_GZIP_ALG						0x0004
#define EXT2_BZIP2_ALG						0x0008
#define EXT2_LZO_ALG						0x0010

/* reserved inode-numbers */
#define EXT2_BAD_INO						1
#define EXT2_ROOT_INO						2
#define EXT2_ACL_IDX_INO					3
#define EXT2_ACL_DATA_INO					4
#define EXT2_BOOT_LOADER_INO				5
#define EXT2_UNDEL_DIR_INO					6

/* mode flags */
/* file format */
#define EXT2_S_IFSOCK						0xC000
#define EXT2_S_IFLNK						0xA000
#define EXT2_S_IFREG						0x8000
#define EXT2_S_IFBLK						0x6000
#define EXT2_S_IFDIR						0x4000
#define EXT2_S_IFCHR						0x2000
#define EXT2_S_IFIFO						0x1000
/* process execution user/group override */
#define EXT2_S_ISUID						0x0800	/* Set process User ID */
#define EXT2_S_ISGID						0x0400	/* Set process Group ID */
#define EXT2_S_ISVTX						0x0200	/* sticky bit */
/* access rights */
#define EXT2_S_IRUSR						0x0100
#define EXT2_S_IWUSR						0x0080
#define EXT2_S_IXUSR						0x0040
#define EXT2_S_IRGRP						0x0020
#define EXT2_S_IWGRP						0x0010
#define EXT2_S_IXGRP						0x0008
#define EXT2_S_IROTH						0x0004
#define EXT2_S_IWOTH						0x0002
#define EXT2_S_IXOTH						0x0001

#define EXT2_S_PERMS						0x0FFF

/* flags */
#define EXT2_SECRM_FL						0x00000001	/* secure deletion */
#define EXT2_UNRM_FL						0x00000002	/* record for undelete */
#define EXT2_COMPR_FL						0x00000004	/* compressed file */
#define EXT2_SYNC_FL						0x00000008	/* synchronous updates */
#define EXT2_IMMUTABLE_FL					0x00000010	/* immutable file */
#define EXT2_APPEND_FL						0x00000020	/* append only */
#define EXT2_NODUMP_FL						0x00000040	/* do not dump/delete file */
#define EXT2_NOATIME_FL						0x00000080	/* do not update .accesstime */
/* for compression: */
#define EXT2_DIRTY_FL						0x00000100
#define EXT2_COMPRBLK_FL					0x00000200	/* compressed blocks */
#define EXT2_NOCOMPR_FL						0x00000400	/* access raw compressed data */
#define EXT2_ECOMPR_FL						0x00000800	/* compression error */
/* compression end */
#define EXT2_BTREE_FL						0x00010000	/* b-tree format directory */
#define EXT2_INDEX_FL						0x00010000  /* hash indexed directory */
#define EXT2_IMAGIC_FL						0x00020000	/* AFS directory */
#define EXT3_JOURNAL_DATA_FL				0x00040000	/* journal file data */
#define EXT2_RESERVED_FL					0x80000000	/* reserved for ext2 library */

class Ext2FileSystem : public FileSystem {
public:
	class Ext2BlockCache : public BlockCache {
	public:
		explicit Ext2BlockCache(Ext2FileSystem *fs)
			: BlockCache(fs->fd,EXT2_BCACHE_SIZE,fs->blockSize()), _fs(fs) {
		}

		virtual bool readBlocks(void *buffer,block_t start,size_t blockCount);
		virtual bool writeBlocks(const void *buffer,size_t start,size_t blockCount);

	private:
		Ext2FileSystem *_fs;
	};

	explicit Ext2FileSystem(const char *device);
	virtual ~Ext2FileSystem();

	virtual inode_t open(FSUser *u,inode_t ino,uint flags);
	virtual void close(inode_t ino);
	virtual inode_t resolve(FSUser *u,const char *path,uint flags);
	virtual int stat(inode_t ino,sFileInfo *info);
	virtual ssize_t read(inode_t ino,void *buffer,off_t offset,size_t size);
	virtual ssize_t write(inode_t ino,const void *buffer,off_t offset,size_t size);
	virtual int link(FSUser *u,inode_t dstIno,inode_t dirIno,const char *name);
	virtual int unlink(FSUser *u,inode_t dirIno,const char *name);
	virtual int mkdir(FSUser *u,inode_t dirIno,const char *name);
	virtual int rmdir(FSUser *u,inode_t dirIno,const char *name);
	virtual int chmod(FSUser *u,inode_t dirIno,mode_t mode);
	virtual int chown(FSUser *u,inode_t dirIno,uid_t uid,gid_t gid);
	virtual void sync();
	virtual void print(FILE *f);

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
	int hasPermission(Ext2CInode *cnode,FSUser *u,uint perms);

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
	block_t getBlockOfInode(inode_t inodeNo) {
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
	block_t getGroupOfInode(inode_t inodeNo) {
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

	/**
	 * @return the current timestamp
	 */
	time_t timestamp() const;

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
	/* for rtc */
	mutable int timeFd;

	/* superblock and blockgroups of that ext2-fs */
	Ext2SBMng sb;
	Ext2BGMng bgs;

	/* caches */
	Ext2INodeCache inodeCache;
	Ext2BlockCache blockCache;
};
