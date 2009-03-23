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
#include "../h/video.h"

#define FILE_ROOT()	(nodes + 3)

/**
 * Creates a pipe
 *
 * @param n the parent-node
 * @param child will be set to the created child
 * @return 0 on success
 */
static s32 vfsn_createPipe(sVFSNode *n,sVFSNode **child);

/**
 * Creates a pipe-node for given process
 *
 * @param pid the process-id
 * @param parent the parent-node
 * @param name the node-name
 * @return the node or NULL
 */
static sVFSNode *vfsn_createPipeNode(tPid pid,sVFSNode *parent,string name);

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
static u32 nextPipeId = 0;

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

bool vfsn_isOwnServiceNode(tVFSNodeNo nodeNo) {
	sProc *p = proc_getRunning();
	sVFSNode *node = nodes + nodeNo;
	return node->owner == p->pid && node->type == T_SERVICE;
}

sVFSNode *vfsn_getNode(tVFSNodeNo nodeNo) {
	return nodes + VIRT_INDEX(nodeNo);
}

string vfsn_getPath(tVFSNodeNo nodeNo) {
	static s8 path[MAX_PATH_LEN];
	u32 nlen,len = 0;
	sVFSNode *node = nodes + nodeNo;
	sVFSNode *n = node;

	ASSERT(node != NULL,"node = NULL");
	/* the root-node of the whole vfs has no path */
	ASSERT(n->parent != NULL,"node->parent == NULL");

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
	while(n->parent->parent != NULL) {
		nlen = strlen(n->name) + 1;
		/* insert the new element */
		*(path + total - nlen) = '/';
		memcpy(path + total + 1 - nlen,n->name,nlen - 1);
		total -= nlen;
		n = n->parent;
	}

	/* now handle the protocol */
	nlen = strlen(n->name) + 1;
	memcpy(path + total - nlen,n->name,nlen - 1);
	*(path + total - 1) = ':';

	/* terminate */
	*(path + len) = '\0';

	return (string)path;
}

s32 vfsn_resolvePath(cstring path,tVFSNodeNo *nodeNo) {
	sVFSNode *n;
	s32 pos;

	/* search for xyz:// */
	pos = strchri(path,':');

	/* we require a complete path here */
	if(*(path + pos) == '\0')
		return ERR_INVALID_PATH;

	/* search our root node */
	n = NODE_FIRST_CHILD(nodes);
	while(n != NULL) {
		if(strncmp(n->name,path,pos) == 0) {
			path += pos + 1;
			break;
		}
		n = n->next;
	}

	/* no matching root found? */
	if(n == NULL)
		return ERR_VFS_NODE_NOT_FOUND;

	/* no absolute path? */
	if(*path != '/' && *path != '\0')
		return ERR_INVALID_PATH;

	/* found file: ? */
	if(n == FILE_ROOT())
		return ERR_REAL_PATH;

	/* skip slashes */
	while(*path == '/')
		path++;

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
	switch(n->type) {
		case T_SERVICE: {
			sVFSNode *child;
			s32 err = vfsn_createServiceUse(n,&child);
			if(err < 0)
				return err;

			/* set new node as resolved one */
			*nodeNo = NADDR_TO_VNNO(child);
			return 0;
		}

		case T_PIPECON: {
			sVFSNode *child;
			s32 err = vfsn_createPipe(n,&child);
			if(err < 0)
				return err;

			*nodeNo = NADDR_TO_VNNO(child);
			return 0;
		}

		case T_LINK:
			/* resolve link */
			n = (sVFSNode*)n->data.def.cache;
			break;
	}

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
	node->owner = INVALID_PID;
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
	/* the root-node and all "protocol-roots" have no parent */
	dotdot->data.def.cache = parent == NULL || parent->parent == NULL ? node : parent;
	return node;
}

sVFSNode *vfsn_createPipeCon(sVFSNode *parent,string name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_PIPECON;
	node->flags = VFS_NOACCESS;
	node->readHandler = NULL;
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

sVFSNode *vfsn_createServiceNode(tPid pid,sVFSNode *parent,string name,u8 type) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	/* TODO */
	node->owner = pid;
	node->type = T_SERVICE;
	node->flags = VFS_NOACCESS | type;
	node->readHandler = NULL;
	return node;
}

sVFSNode *vfsn_createServiceUseNode(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = proc_getRunning()->pid;
	node->type = T_SERVUSE;
	node->flags = VFS_READ | VFS_WRITE;
	node->readHandler = handler;
	node->data.servuse.locked = INVALID_PID;
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

s32 vfsn_createServiceUse(sVFSNode *n,sVFSNode **child) {
	string name;
	sVFSNode *m;
	sProc *p = proc_getRunning();

	if(n->flags & VFS_SINGLEPIPE) {
		/* check if the node does already exist */
		m = NODE_FIRST_CHILD(n);
		while(m != NULL) {
			if(strcmp(m->name,SERVICE_CLIENT_ALL) == 0) {
				*child = m;
				return 0;
			}
			m = m->next;
		}

		name = (string)SERVICE_CLIENT_ALL;
	}
	else {
		/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
		name = kheap_alloc(12 * sizeof(s8));
		if(name == NULL)
			return ERR_NOT_ENOUGH_MEM;

		/* create usage-node */
		itoa(name,p->pid);

		/* check duplicate usage */
		m = NODE_FIRST_CHILD(n);
		while(m != NULL) {
			if(strcmp(m->name,name) == 0) {
				kheap_free(name);
				*child = m;
				return 0;
			}
			m = m->next;
		}
	}

	/* ok, create a service-usage-node */
	m = vfsn_createServiceUseNode(n,name,vfs_serviceUseReadHandler);
	if(m == NULL) {
		if((n->flags & VFS_SINGLEPIPE) == 0)
			kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	*child = m;
	return 0;
}

static s32 vfsn_createPipe(sVFSNode *n,sVFSNode **child) {
	string name;
	sVFSNode *m;
	u32 len;
	sProc *p = proc_getRunning();

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	/* we want to have to form <pid>.<x>, therefore two ints and a '.' */
	name = kheap_alloc((11 * 2 + 2) * sizeof(s8));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create usage-node */
	itoa(name,p->pid);
	len = strlen(name);
	*(name + len) = '.';
	itoa(name + len + 1,nextPipeId++);

	/* ok, create a pipe-node */
	m = vfsn_createPipeNode(p->pid,n,name);
	if(m == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	*child = m;
	return 0;
}

static sVFSNode *vfsn_createPipeNode(tPid pid,sVFSNode *parent,string name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	node->type = T_PIPE;
	node->flags = VFS_READ | VFS_WRITE;
	node->readHandler = vfs_defReadHandler;
	return node;
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
		vid_printf("\towner: %d\n",node->owner);
		if(node->type == T_SERVUSE) {
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
