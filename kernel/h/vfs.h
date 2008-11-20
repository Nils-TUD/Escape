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

void vfs_init(void);

void vfs_printTree(void);

tVFSNode *vfs_createDir(tVFSNode *parent,tVFSNode *prev,string name);

tVFSNode *vfs_createInfo(tVFSNode *parent,tVFSNode *prev,string name,void *data,u16 dataSize);

tVFSNode *vfs_createService(tVFSNode *parent,tVFSNode *prev,string name);

tVFSNode *vfs_createNode(tVFSNode *parent,tVFSNode *prev,string name);

#endif /* VFS_H_ */
