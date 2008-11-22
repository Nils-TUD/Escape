/**
 * @version		$Id: sched.c 73 2008-11-20 13:21:59Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VFS_H_
#define VFS_H_

#include "../h/common.h"

/* the possible node-types */
typedef enum {T_DIR,T_INFO,T_SERVICE} tNodeType;

/* flags for the GFTEntries */
enum {GFT_READ = 1,GFT_WRITE = 2,GFT_VIRTUAL = 4,GFT_REAL = 8};

/* a node in our virtual file system */
typedef struct tVFSNode tVFSNode;
struct tVFSNode {
	string name;
	u16 type;
	u16 dataSize;
	void *data;
	tVFSNode *next;
	tVFSNode *childs;
};

/**
 * Initializes the virtual file system
 */
void vfs_init(void);

/**
 * @param index the node-number
 * @return the node for given index
 */
tVFSNode *vfs_getNode(u32 index);

/**
 * Opens the file with given number and given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processes may read from the same file simultaneously but NOT write!
 *
 * @param flags wether it is a virtual or real file and wether you want to read or write
 * @param nodeNo the node-number (in the virtual or real environment)
 * @return the file-descriptor or < 0 (ERR_FILE_IN_USE, ERR_NO_FREE_FD) if an error occurred
 */
s32 vfs_openFile(u8 flags,u32 nodeNo);

/**
 * Closes the given file-descriptor. That means it decrements the reference-count in the
 * global file table. If there are no references anymore it releases the slot.
 *
 * @param fd the file-descriptor
 */
void vfs_closeFile(u32 fd);

void vfs_printGFT(void);

void vfs_printTree(void);

void vfs_printNode(tVFSNode *node);

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
 * @param isReal will contain wether the node for that path is a real or virtual node
 * @return the node-number or < 0 if not found
 */
s32 vfs_resolvePath(cstring path,bool *isReal);

tVFSNode *vfs_createDir(tVFSNode *parent,tVFSNode *prev,string name);

tVFSNode *vfs_createInfo(tVFSNode *parent,tVFSNode *prev,string name,void *data,u16 dataSize);

tVFSNode *vfs_createService(tVFSNode *parent,tVFSNode *prev,string name);

tVFSNode *vfs_createNode(tVFSNode *parent,tVFSNode *prev,string name);

#endif /* VFS_H_ */
