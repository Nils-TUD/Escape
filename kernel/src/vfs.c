/**
 * @version		$Id: sched.c 73 2008-11-20 13:21:59Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/proc.h"
#include "../h/video.h"
#include "../h/util.h"

/*
 * dirs: /, /fs, /system, /system/processes, /system/services
 * files: /system/processes/%, /system/services/%
 */
#define NODE_COUNT (PROC_COUNT * 8 + 4 + 64)

/* all nodes */
static tVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static tVFSNode *freeList;

/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static tVFSNode *vfs_requestNode(void) {
	tVFSNode *node;
	if(freeList == NULL)
		panic("No free vfs-nodes!");

	node = freeList;
	freeList = freeList->next;
	return node;
}

/**
 * Releases the given node
 *
 * @param node the node
 */
static void vfs_releaseNode(tVFSNode *node) {
	node->next = freeList;
	freeList = node;
}

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfs_doPrintTree(u32 level,tVFSNode *parent) {
	u32 i;
	tVFSNode *n = parent->childs;
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		vfs_doPrintTree(level + 1,n);
		n = n->next;
	}
}

void vfs_init(void) {
	u32 i;
	tVFSNode *root,*sys,*node = &nodes[0];
	freeList = node;
	for(i = 0; i < NODE_COUNT - 1; i++) {
		node->next = node + 1;
		node++;
	}
	node->next = NULL;

	/*
	 *  /
	 *   |-fs
	 *   \-system
	 *     |-processes
	 *     \-services
	 */
	root = vfs_createDir(NULL,NULL,(string)"");
	node = vfs_createService(root,NULL,(string)"fs");
	sys = vfs_createDir(root,node,(string)"system");
	node = vfs_createDir(sys,NULL,(string)"processes");
	node = vfs_createDir(sys,node,(string)"services");
}

void vfs_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfs_doPrintTree(1,&nodes[0]);
}

tVFSNode *vfs_createDir(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_DIR;
	node->dataSize = 0;
	node->data = NULL;
	return node;
}

tVFSNode *vfs_createInfo(tVFSNode *parent,tVFSNode *prev,string name,void *data,u16 dataSize) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_INFO;
	node->dataSize = dataSize;
	node->data = data;
	return node;
}

tVFSNode *vfs_createService(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	/* TODO */
	node->type = T_DIR;
	node->dataSize = 0;
	node->data = NULL;
	return node;
}

tVFSNode *vfs_createNode(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_requestNode();
	node->name = name;
	node->next = NULL;
	node->childs = NULL;

	if(prev != NULL)
		prev->next = node;
	else if(parent != NULL)
		parent->childs = node;
	return node;
}
