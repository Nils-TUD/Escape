/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VFS_H_
#define VFS_H_

#include "../h/common.h"

/* the possible node-types */
typedef enum {T_DIR,T_INFO,T_SERVICE} eNodeType;

/* flags for the GFTEntries */
enum {GFT_READ = 1,GFT_WRITE = 2};

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;
/* the function for read-requests on info-nodes */
typedef s32 (*fRead)(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

struct sVFSNode {
	string name;
	fRead readHandler;
	u16 type;
	u16 dataSize;
	void *dataCache;
	sVFSNode *next;
	sVFSNode *childs;
};

/**
 * Initializes the virtual file system
 */
void vfs_init(void);

/**
 * @param nodeNo the node-number
 * @return the node for given index
 */
sVFSNode *vfs_getNode(tVFSNodeNo nodeNo);

/**
 * Opens the file with given number and given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processes may read from the same file simultaneously but NOT write!
 *
 * @param flags wether it is a virtual or real file and wether you want to read or write
 * @param nodeNo the node-number (in the virtual or real environment)
 * @param fd will be set to the file-descriptor if successfull
 * @return 0 if successfull or < 0 (ERR_FILE_IN_USE, ERR_NO_FREE_FD)
 */
s32 vfs_openFile(u8 flags,tVFSNodeNo nodeNo,tFD *fd);

/**
 * Reads max. count bytes from the given fd into the given buffer and returns the number
 * of read bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to write to
 * @param count the max. number of bytes to read
 * @return the number of bytes read
 */
s32 vfs_readFile(tFD fd,u8 *buffer,u32 count);

/**
 * Closes the given fd. That means it calls proc_closeFile() and decrements the reference-count
 * in the global file table. If there are no references anymore it releases the slot.
 *
 * @param fd the file-descriptor
 */
void vfs_closeFile(tFD fd);

void vfs_printGFT(void);

void vfs_printTree(void);

void vfs_printNode(sVFSNode *node);

/**
 * @return the number of entries in the global file table
 */
u32 vfs_getGFTEntryCount(void);

/**
 * Cleans the given path. That means multiple slashes are removed and "." and ".." are resolved.
 * After the call you can be sure that you have a "clean" path.
 *
 * @param path the path to clean
 * @return the clean path
 */
string vfs_cleanPath(string path);

/**
 * Resolves the given path to a VFS-node
 *
 * @param path the CLEAN path to resolve
 * @param nodeNo the node-number for that path (will be set)
 * @return 0 if successfull or the error-code
 */
s32 vfs_resolvePath(cstring path,tVFSNodeNo *nodeNo);

/**
 * Creates a process-node with given pid and handler-function
 *
 * @param pid the process-id
 * @param handler the read-handler
 * @return true if successfull
 */
bool vfs_createProcessNode(tPid pid,fRead handler);

/**
 * Removes the node for the process with given id from the VFS
 *
 * @param pid the process-id
 */
void vfs_removeProcessNode(tPid pid);

#endif /* VFS_H_ */
