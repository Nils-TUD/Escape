/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SUPERBLOCK_H_
#define SUPERBLOCK_H_

#include <common.h>

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
	s8 volumeName[16];
	/* 64 bytes directory path where the file system was last mounted. */
	s8 lastMountPath[64];
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

#endif /* SUPERBLOCK_H_ */
