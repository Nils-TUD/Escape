/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef INODE_H_
#define INODE_H_

#include <common.h>
#include "ext2.h"

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
	u32 inode;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} __attribute__((packed)) sDirEntry;

/**
 * Retrieves the inode-data of the inode with given number.
 *
 * @param e the ext2-handle
 * @param no the inode-number
 * @return the inode or NULL if not found
 */
sInode *ext2_getInode(sExt2 *e,tInodeNo no);

/**
 * Determines the inode-number IN the corresponding group
 *
 * @param e the ext2-handle
 * @param no the inode-number
 * @return the inode-number in the group
 */
u32 ext2_getGroupInodeNo(sExt2 *e,tInodeNo no);

/**
 * Determines the group of the given inode-number
 *
 * @param e the ext2-handle
 * @param no the inode-number
 * @return the group-number
 */
u32 ext2_getInodeGroup(sExt2 *e,tInodeNo no);

#if DEBUGGING

/**
 * Prints the given inode
 *
 * @param inode the inode
 */
void ext2_dbg_printInode(sInode *inode);

#endif

#endif /* INODE_H_ */
