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

#ifndef EXT2_H_
#define EXT2_H_

#include <esc/common.h>
#include <sllist.h>

#define SECTOR_SIZE							512
#define BLOCK_SIZE(e)						(SECTOR_SIZE << ((e)->superBlock.logBlockSize + 1))
#define BLOCKS_TO_SECS(e,x)					((x) << ((e)->superBlock.logBlockSize + 1))
#define SECS_TO_BLOCKS(e,x)					((x) >> ((e)->superBlock.logBlockSize + 1))
#define BYTES_TO_BLOCKS(e,b)				(((b) + (BLOCK_SIZE(e) - 1)) / BLOCK_SIZE(e))

/* first sector of the super-block in the first block-group */
#define EXT2_SUPERBLOCK_SECNO				2
/* first block-number of the block-group-descriptor-table in other block-groups than the first */
#define EXT2_BLOGRPTBL_BNO					2

/* number of direct blocks in the inode */
#define EXT2_DIRBLOCK_COUNT					12

#define INODE_CACHE_SIZE					64
#define BLOCK_CACHE_SIZE					256

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


typedef struct {
	/* the total number of inodes, both used and free, in the file system. */
	u32 inodeCount;
	/* the total number of blocks in the system including all used, free and reserved */
	u32 blockCount;
	/* number of blocks reserved for the super-user */
	u32 suResBlockCount;
	u32 freeBlockCount;
	u32 freeInodeCount;
	u32 firstDataBlock;
	/* blockSize = 1024 << logBlockSize; */
	u32 logBlockSize;
	/* if(positive)
	 *   fragmentSize = 1024 << logFragSize;
	 * else
	 *   fragmentSize = 1024 >> -logFragSize; */
	u32 logFragSize;
	u32 blocksPerGroup;
	u32 fragsPerGroup;
	u32 inodesPerGroup;
	u32 lastMountTime;
	u32 lastWritetime;
	/* number of mounts since last verification */
	u16 mountCount;
	/* max number of mounts until a full check has to be performed */
	u16 maxMountCount;
	/* should be EXT2_SUPER_MAGIC */
	u16 magic;
	/* EXT2_VALID_FS or EXT2_ERROR_FS */
	u16 state;
	/* on of EXT2_ERRORS_* */
	u16 errors;
	u16 minorRevLevel;
	u32 lastCheck;
	u32 checkInterval;
	u32 creatorOS;
	u32 revLevel;
	/* the default user-/group-id for reserved blocks */
	u16 defResUid;
	u16 defResGid;
	/* the index to the first inode useable for standard files. Always EXT2_REV0_FIRST_INO for
	 * revision 0. In revision 1 and later this value may be set to any value. */
	u32 firstInode;
	/* 16bit value indicating the size of the inode structure. In revision 0, this value is always
	 * 128 (EXT2_REV0_INODE_SIZE). In revision 1 and later, this value must be a perfect power of
	 * 2 and must be smaller or equal to the block size (1<<s_log_block_size). */
	u16 inodeSize;
	/* the block-group which hosts this super-block */
	u16 blockGroupNo;
	/* 32bit bitmask of compatible features */
	u32 featureCompat;
	/* 32bit bitmask of incompatible features */
	u32 featureInCompat;
	/* 32bit bitmask of “read-only” features. The file system implementation should mount as
	 * read-only if any of the indicated feature is unsupported. */
	u32 featureRoCompat;
	/* 128bit value used as the volume id. This should, as much as possible, be unique for
	 * each file system formatted. */
	u8 volumeUid[16];
	/* 16 bytes volume name, mostly unusued. A valid volume name would consist of only ISO-Latin-1
	 * characters and be 0 terminated. */
	char volumeName[16];
	/* 64 bytes directory path where the file system was last mounted. */
	char lastMountPath[64];
	/* 32bit value used by compression algorithms to determine the compression method(s) used. */
	u32 algoBitmap;
	/* 8-bit value representing the number of blocks the implementation should attempt to
	 * pre-allocate when creating a new regular file. */
	u8 preAllocBlocks;
	/* 8-bit value representing the number of blocks the implementation should attempt to
	 * pre-allocate when creating a new directory. */
	u8 preAllocDirBlocks;
	/* alignment */
	u16 : 16;
	/* 16-byte value containing the uuid of the journal superblock */
	u8 journalUid[16];
	/* 32-bit inode number of the journal file */
	u32 journalInodeNo;
	/* 32-bit device number of the journal file */
	u32 journalDev;
	/* 32-bit inode number, pointing to the first inode in the list of inodes to delete. */
	u32 lastOrphan;
	/* An array of 4 32bit values containing the seeds used for the hash algorithm for directory
	 * indexing. */
	u32 hashSeed[4];
	/* An 8bit value containing the default hash version used for directory indexing.*/
	u8 defHashVersion;
	/* padding */
	u16 : 16;
	u8 : 8;
	/* A 32bit value containing the default mount options for this file system. */
	u32 defMountOptions;
	/* A 32bit value indicating the block group ID of the first meta block group. */
	u32 firstMetaBg;
	/* UNUSED */
	u8 unused[760];
} __attribute__((packed)) sSuperBlock;

typedef struct {
	/* block id of the first block of the "block bitmap" for the group represented. */
	u32 blockBitmap;
	/* block id of the first block of the "inode bitmap" for the group represented. */
	u32 inodeBitmap;
	/* block id of the first block of the "inode table" for the group represented. */
	u32 inodeTable;
	u16 freeBlockCount;
	u16 freeInodeCount;
	/* the number of inodes allocated to directories for the represented group. */
	u16 usedDirCount;
	/* padding */
	u16 : 16;
	/* reserved */
	u32 : 32;
	u32 : 32;
	u32 : 32;
} __attribute__((packed)) sBlockGroup;

typedef struct {
	u16 mode;
	u16 uid;
	s32 size;
	u32 accesstime;
	u32 createtime;
	u32 modifytime;
	u32 deletetime;
	u16 gid;
	/* number of links to this inode (when it reaches 0 the inode and all its associated blocks
	 * are freed) */
	u16 linkCount;
	/* the total number of blocks reserved to contain the data of this inode, regardless
	 * if these blocks are used or not. */
	u32 blocks;
	u32 flags;
	/* OS dependant value. */
	u32 osd1;
	/* A value of 0 in the block-array effectively terminates it with no further block being defined.
	 * All the remaining entries of the array should still be set to 0. */
	u32 dBlocks[12];
	u32 singlyIBlock;
	u32 doublyIBlock;
	u32 triplyIBlock;
	/* used to indicate the file version (used by NFS). */
	u32 generation;
	/* indicating the block number containing the extended attributes. In revision 0 this value
	 * is always 0. */
	u32 fileACL;
	/* In revision 0 this 32bit value is always 0. In revision 1, for regular files this 32bit
	 * value contains the high 32 bits of the 64bit file size. */
	u32 dirACL;
	/* 32bit value indicating the location of the file fragment. */
	u32 fragAddr;
	/* 96bit OS dependant structure. */
	u16 osd2[6];
} __attribute__((packed)) sInode;

typedef struct {
	tInodeNo inode;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
	char name[];
} __attribute__((packed)) sDirEntry;

typedef struct {
	tInodeNo inodeNo;
	u16 dirty;
	u16 refs;
	sInode inode;
} sCachedInode;

typedef struct {
	u32 blockNo;
	u8 dirty;
	/* NULL indicates an unused entry */
	u8 *buffer;
} sBCacheEntry;

typedef struct {
	/* the file-desc for ATA */
	tFD ataFd;

	/* superblock and blockgroups of that ext2-fs */
	bool sbDirty;
	sSuperBlock superBlock;
	bool groupsDirty;
	sBlockGroup *groups;

	/* caches */
	sCachedInode inodeCache[INODE_CACHE_SIZE];
	u32 blockCacheFree;
	sBCacheEntry blockCache[BLOCK_CACHE_SIZE];
} sExt2;

/**
 * Inits the given ext2-filesystem
 *
 * @param e the ext2-data
 * @return true if successfull
 */
bool ext2_init(sExt2 *e);

/**
 * Destroys the given ext2-filesystem
 *
 * @param e the ext2-data
 */
void ext2_destroy(sExt2 *e);


#if DEBUGGING

/**
 * Prints all block-groups
 *
 * @param e the ext2-data
 */
void ext2_dbg_printBlockGroups(sExt2 *e);

#endif

#endif /* EXT2_H_ */
