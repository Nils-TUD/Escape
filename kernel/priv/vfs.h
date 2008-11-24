/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVVFS_H_
#define PRIVVFS_H_

#include "../pub/common.h"
#include "../pub/vfs.h"

/*
 * dirs: /, /fs, /system, /system/processes, /system/services
 * files: /system/processes/%, /system/services/%
 */
#define NODE_COUNT	(PROC_COUNT * 8 + 4 + 64)
/* max number of open files */
#define FILE_COUNT	(PROC_COUNT * 16)
/* the processes node */
#define PROCESSES()	(nodes + 3)

/* max node-name len */
#define MAX_NAME_LEN 59

/* determines the node-number (for a virtual node) from the given node-address */
#define NADDR_TO_VNNO(naddr) ((((u32)(naddr) - (u32)&nodes[0]) / sizeof(sVFSNode)) | (1 << 31))

/* checks wether the given node-number is a virtual one */
#define IS_VIRT(nodeNo) (((nodeNo) & (1 << 31)) != 0)
/* makes a virtual node number */
#define MAKE_VIRT(nodeNo) ((nodeNo) | (1 << 31))
/* removes the virtual-flag */
#define VIRT_INDEX(nodeNo) ((nodeNo) & ~(1 << 31))

/* the public VFS-node (passed to the user) */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 name[MAX_NAME_LEN + 1];
} sVFSNodePub;

/* an entry in the global file table */
typedef struct {
	/* read OR write; flags = 0 => entry unused */
	u8 flags;
	/* number of references */
	u16 refCount;
	/* current position in file */
	u32 position;
	/* node-number; if MSB = 1 => virtual, otherwise real (fs) */
	tVFSNodeNo nodeNo;
} sGFTEntry;

/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static sVFSNode *vfs_requestNode(void);

/**
 * Releases the given node
 *
 * @param node the node
 */
static void vfs_releaseNode(sVFSNode *node);

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfs_doPrintTree(u32 level,sVFSNode *parent);

/**
 * The read-handler for directories
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
static s32 vfs_dirReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * Creates a (incomplete) node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createNode(sVFSNode *parent,sVFSNode *prev,string name);

/**
 * Creates a directory-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createDir(sVFSNode *parent,sVFSNode *prev,string name);

/**
 * Creates an info-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param handler the read-handler
 * @return the node
 */
static sVFSNode *vfs_createInfo(sVFSNode *parent,sVFSNode *prev,string name,fRead handler);

/**
 * Creates a service-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createService(sVFSNode *parent,sVFSNode *prev,string name);

#endif /* PRIVVFS_H_ */
