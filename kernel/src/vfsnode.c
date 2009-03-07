/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "../h/util.h"
#include "../h/kheap.h"
#include <string.h>
#include <video.h>

/**
 * Appends a service-usage-node to the given node and stores the pointer to the new node
 * at <child>.
 *
 * @param n the node to which the new node should be appended
 * @param child will contain the pointer to the new node, if successfull
 * @return the error-code if negative or 0 if successfull
 */
static s32 vfsn_createServiceUse(sVFSNode *n,sVFSNode **child);

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfsn_doPrintTree(u32 level,sVFSNode *parent);

/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static sVFSNode *vfsn_requestNode(void);

/**
 * Releases the given node
 *
 * @param node the node
 */
static void vfsn_releaseNode(sVFSNode *node);


/* all nodes */
sVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static sVFSNode *freeList;

void vfsn_init(void) {
	tVFSNodeNo i;
	sVFSNode *node = &nodes[0];
	freeList = node;
	for(i = 0; i < NODE_COUNT - 1; i++) {
		node->next = node + 1;
		node++;
	}
	node->next = NULL;
}

bool vfsn_isValidNodeNo(tVFSNodeNo nodeNo) {
	if(IS_VIRT(nodeNo))
		return VIRT_INDEX(nodeNo) < NODE_COUNT;
	return nodeNo < NODE_COUNT;
}

bool vfsn_isOwnServiceNode(sVFSNode *node) {
	sProc *p = proc_getRunning();
	return node->data.service.proc == p && node->type == T_SERVICE;
}

sVFSNode *vfsn_getNode(tVFSNodeNo nodeNo) {
	return nodes + nodeNo;
}

string vfsn_getPath(sVFSNode *node) {
	static s8 path[MAX_PATH_LEN];
	u32 nlen,len = 0;
	sVFSNode *n = node;
	ASSERT(node != NULL,"node = NULL");

	/* '/' is a special case */
	if(n->parent == NULL) {
		*path = '/';
		len = 1;
	}
	else {
		u32 total = 0;
		while(n->parent != NULL) {
			/* name + slash */
			total += strlen(n->name) + 1;
			n = n->parent;
		}

		/* not nice, but ensures that we don't overwrite something :) */
		if(total > MAX_PATH_LEN)
			total = MAX_PATH_LEN;

		n = node;
		len = total;
		while(n->parent != NULL) {
			nlen = strlen(n->name) + 1;
			/* move the current string forward */
			/* insert the new element */
			*(path + total - nlen) = '/';
			memcpy(path + total + 1 - nlen,n->name,nlen - 1);
			total -= nlen;
			n = n->parent;
		}
	}

	/* terminate */
	*(path + len) = '\0';

	return (string)path;
}

s32 vfsn_resolvePath(cstring path,tVFSNodeNo *nodeNo) {
	sVFSNode *n;
	s32 pos;
	/* select start */
	if(*path == '/') {
		n = &nodes[0];
		/* skip slashes (we have at least 1) */
		do {
			path++;
		}
		while(*path == '/');
	}
	else
		panic("TODO: use current path");

	/* root/current node requested? */
	if(!*path) {
		*nodeNo = NADDR_TO_VNNO(n);
		return 0;
	}

	pos = strchri(path,'/');
	n = NODE_FIRST_CHILD(n);
	while(n != NULL) {
		if(strncmp(n->name,path,pos) == 0) {
			path += pos;
			/* finished? */
			if(!*path)
				break;

			/* skip slashes */
			while(*path == '/') {
				path++;
			}
			/* "/" at the end is optional */
			if(!*path)
				break;

			/* TODO ?? */
			if(n->type == T_SERVICE)
				break;

			/* move to childs of this node */
			pos = strchri(path,'/');
			n = NODE_FIRST_CHILD(n);
			continue;
		}
		n = n->next;
	}

	if(n == NULL)
		return ERR_VFS_NODE_NOT_FOUND;

	/* handle service */
	if(n->type == T_SERVICE) {
		sVFSNode *child;
		s32 err = vfsn_createServiceUse(n,&child);
		if(err < 0)
			return err;

		/* set new node as resolved one */
		*nodeNo = NADDR_TO_VNNO(child);
		return 0;
	}

	/* resolve link */
	if(n->type == T_LINK)
		n = (sVFSNode*)n->data.def.cache;

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

sVFSNode *vfsn_createNodeAppend(sVFSNode *parent,string name) {
	sVFSNode *node = vfsn_createNode(parent,name);
	vfsn_appendChild(parent,node);
	return node;
}

sVFSNode *vfsn_createNode(sVFSNode *parent,string name) {
	sVFSNode *node;

	ASSERT(name != NULL,"name == NULL");

	if(strlen(name) > MAX_NAME_LEN)
		return NULL;

	node = vfsn_requestNode();
	if(node == NULL)
		return NULL;

	/* ensure that all values are initialized properly */
	node->name = name;
	node->refCount = 0;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->data.def.cache = NULL;
	node->data.def.size = 0;
	node->data.def.pos = 0;
	return node;
}

sVFSNode *vfsn_createDir(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	sVFSNode *dot = vfsn_createNodeAppend(node,(string)".");
	if(dot == NULL)
		return NULL;
	sVFSNode *dotdot = vfsn_createNodeAppend(node,(string)"..");
	if(dotdot == NULL)
		return NULL;

	node->type = T_DIR;
	node->flags = VFS_READ;
	node->readHandler = handler;
	dot->type = T_LINK;
	dot->data.def.cache = node;
	dotdot->type = T_LINK;
	dotdot->data.def.cache = parent == NULL ? node : parent;
	return node;
}

sVFSNode *vfsn_createInfo(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_INFO;
	node->flags = VFS_READ;
	node->readHandler = handler;
	return node;
}

sVFSNode *vfsn_createServiceNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	/* TODO */
	node->type = T_SERVICE;
	node->flags = VFS_NOACCESS;
	node->readHandler = NULL;
	return node;
}

sVFSNode *vfsn_createServiceUseNode(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_SERVUSE;
	node->flags = VFS_READ | VFS_WRITE;
	node->readHandler = handler;
	return node;
}

void vfsn_appendChild(sVFSNode *parent,sVFSNode *node) {
	ASSERT(node != NULL,"node == NULL");

	if(parent != NULL) {
		if(parent->firstChild == NULL)
			parent->firstChild = node;
		if(parent->lastChild != NULL)
			parent->lastChild->next = node;
		node->prev = parent->lastChild;
		parent->lastChild = node;
	}
	node->parent = parent;
}

void vfsn_removeChild(sVFSNode *parent,sVFSNode *node) {
	ASSERT(parent != NULL,"parent == NULL");
	ASSERT(node != NULL,"node == NULL");

	if(node->prev != NULL)
		node->prev->next = node->next;
	else
		parent->firstChild = node->next;
	if(node->next != NULL)
		node->next->prev = node->prev;
	else
		parent->lastChild = node->prev;

	vfsn_releaseNode(node);
}

static s32 vfsn_createServiceUse(sVFSNode *n,sVFSNode **child) {
	string name;
	sVFSNode *m;
	sProc *p = proc_getRunning();

	/* TODO we should check for duplicates and if so just reserve a different file for it */

	/* check duplicate usage
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		if(m->data.service.proc == p)
			return ERR_PROC_DUP_SERVICE_USE;
		m = m->next;
	}*/

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	name = kheap_alloc(12 * sizeof(s8));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create usage-node */
	itoa(name,p->pid);
	m = vfsn_createServiceUseNode(n,name,vfs_serviceUseReadHandler);
	if(m == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	/*m->data.service.proc = p;*/
	*child = m;
	return 0;
}

static sVFSNode *vfsn_requestNode(void) {
	sVFSNode *node;
	if(freeList == NULL)
		panic("No free vfs-nodes!");

	node = freeList;
	freeList = freeList->next;
	return node;
}

static void vfsn_releaseNode(sVFSNode *node) {
	ASSERT(node != NULL,"node == NULL");
	/* mark unused */
	node->name = NULL;
	node->next = freeList;
	freeList = node;
}

static void vfsn_doPrintTree(u32 level,sVFSNode *parent) {
	u32 i;
	sVFSNode *n = NODE_FIRST_CHILD(parent);
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		/* don't recurse for "." and ".." */
		if(strncmp(n->name,".",1) != 0 && strncmp(n->name,"..",2) != 0)
			vfsn_doPrintTree(level + 1,n);
		n = n->next;
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void vfsn_dbg_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfsn_doPrintTree(1,&nodes[0]);
}

void vfsn_dbg_printNode(sVFSNode *node) {
	vid_printf("VFSNode @ 0x%x:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name);
		vid_printf("\ttype: %s\n",node->type == T_DIR ? "DIR" : node->type == T_INFO ? "INFO" : "SERVICE");
		vid_printf("\tfirstChild: 0x%x\n",node->firstChild);
		vid_printf("\tlastChild: 0x%x\n",node->lastChild);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\tprev: 0x%x\n",node->prev);
		if(node->type == T_SERVICE)
			vid_printf("\tProcess: 0x%x\n",node->data.service.proc);
		else if(node->type == T_SERVUSE) {
			vid_printf("\tSendList: 0x%x\n",node->data.servuse.sendList);
			vid_printf("\tRecvList: 0x%x\n",node->data.servuse.recvList);
		}
		else {
			vid_printf("\treadHandler: 0x%x\n",node->readHandler);
			vid_printf("\tcache: 0x%x\n",node->data.def.cache);
			vid_printf("\tsize: %d\n",node->data.def.size);
			vid_printf("\tpos: %d\n",node->data.def.pos);
		}
	}
}

#endif
