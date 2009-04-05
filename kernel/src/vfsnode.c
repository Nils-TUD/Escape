/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <vfs.h>
#include <vfsnode.h>
#include <vfsinfo.h>
#include <util.h>
#include <kheap.h>
#include <video.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

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
static sVFSNode *vfsn_createPipeNode(tPid pid,sVFSNode *parent,char *name);

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
	return node->owner == p->pid && (node->mode & MODE_TYPE_SERVICE);
}

sVFSNode *vfsn_getNode(tVFSNodeNo nodeNo) {
	return nodes + VIRT_INDEX(nodeNo);
}

s32 vfsn_getNodeInfo(tVFSNodeNo nodeNo,sFileInfo *info) {
	sVFSNode *n = nodes + nodeNo;

	if(n->mode == 0)
		return ERR_INVALID_NODENO;

	/* some infos are not available here */
	/* TODO a lot of is missing here */
	info->device = 0;
	info->rdevice = 0;
	info->accesstime = 0;
	info->modifytime = 0;
	info->createtime = 0;
	info->blockCount = 0;
	info->blockSize = 512;
	info->inodeNo = nodeNo;
	info->linkCount = 0;
	info->uid = 0;
	info->gid = 0;
	info->mode = n->mode;
	if((n->mode & MODE_TYPE_SERVUSE))
		info->size = 0;
	else
		info->size = n->data.def.pos;
	return 0;
}

char *vfsn_getPath(tVFSNodeNo nodeNo) {
	static char path[MAX_PATH_LEN];
	u32 nlen,len = 0;
	sVFSNode *node = nodes + nodeNo;
	sVFSNode *n = node;

	vassert(node != NULL,"node = NULL");
	/* the root-node of the whole vfs has no path */
	vassert(n->parent != NULL,"node->parent == NULL");

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

	return (char*)path;
}

s32 vfsn_resolvePath(const char *path,tVFSNodeNo *nodeNo) {
	sVFSNode *n;
	s32 pos;

	/* search for xyz:// */
	pos = strchri(path,':');

	/* we require a complete path here */
	if(pos == 0 || *(path + pos) == '\0')
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
		if((s32)strlen(n->name) == pos && strncmp(n->name,path,pos) == 0) {
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

			if(n->mode & MODE_TYPE_SERVICE)
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

	/* handle special node-types */
	if((n->mode & MODE_TYPE_SERVICE)) {
		sVFSNode *child;
		s32 err = vfsn_createServiceUse(n,&child);
		if(err < 0)
			return err;

		/* set new node as resolved one */
		*nodeNo = NADDR_TO_VNNO(child);
		return 0;
	}
	if((n->mode & MODE_TYPE_PIPECON)) {
		sVFSNode *child;
		s32 err = vfsn_createPipe(n,&child);
		if(err < 0)
			return err;

		*nodeNo = NADDR_TO_VNNO(child);
		return 0;
	}
	if(MODE_IS_LINK(n->mode)) {
		/* resolve link */
		n = (sVFSNode*)n->data.def.cache;
	}

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

sVFSNode *vfsn_createNodeAppend(sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNode(parent,name);
	vfsn_appendChild(parent,node);
	return node;
}

sVFSNode *vfsn_createNode(sVFSNode *parent,char *name) {
	sVFSNode *node;

	vassert(name != NULL,"name == NULL");

	if(strlen(name) > MAX_NAME_LEN)
		return NULL;

	node = vfsn_requestNode();
	if(node == NULL)
		return NULL;

	/* ensure that all values are initialized properly */
	node->name = name;
	node->mode = 0;
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

sVFSNode *vfsn_createDir(sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	sVFSNode *dot = vfsn_createNodeAppend(node,(char*)".");
	if(dot == NULL)
		return NULL;
	sVFSNode *dotdot = vfsn_createNodeAppend(node,(char*)"..");
	if(dotdot == NULL)
		return NULL;

	node->mode = MODE_TYPE_DIR | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OWNER_EXEC |
		MODE_OTHER_READ | MODE_OTHER_EXEC;
	node->readHandler = vfsinfo_dirReadHandler;
	dot->mode = MODE_TYPE_LINK | MODE_OWNER_READ | MODE_OTHER_READ;
	dot->data.def.cache = node;
	dotdot->mode = MODE_TYPE_LINK | MODE_OWNER_READ | MODE_OTHER_READ;
	/* the root-node and all "protocol-roots" have no parent */
	dotdot->data.def.cache = parent == NULL || parent->parent == NULL ? node : parent;
	return node;
}

sVFSNode *vfsn_createPipeCon(sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->mode = MODE_TYPE_PIPECON | MODE_OWNER_READ | MODE_OWNER_WRITE;
	node->readHandler = NULL;
	return node;
}

sVFSNode *vfsn_createInfo(tPid pid,sVFSNode *parent,char *name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	node->mode = MODE_TYPE_FILE | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OTHER_READ;
	node->readHandler = handler;
	return node;
}

sVFSNode *vfsn_createServiceNode(tPid pid,sVFSNode *parent,char *name,u32 type) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	node->mode = MODE_TYPE_SERVICE | MODE_OWNER_READ | MODE_OTHER_READ | type;
	node->readHandler = NULL;
	return node;
}

sVFSNode *vfsn_createServiceUseNode(sVFSNode *parent,char *name,fRead handler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = proc_getRunning()->pid;
	node->mode = MODE_TYPE_SERVUSE | MODE_OWNER_READ | MODE_OWNER_WRITE |
		MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->readHandler = handler;
	node->data.servuse.locked = -1;
	return node;
}

void vfsn_appendChild(sVFSNode *parent,sVFSNode *node) {
	vassert(node != NULL,"node == NULL");

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
	vassert(parent != NULL,"parent == NULL");
	vassert(node != NULL,"node == NULL");

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
	char *name;
	sVFSNode *m;
	sProc *p = proc_getRunning();

	if(n->mode & MODE_SERVICE_SINGLEPIPE) {
		/* check if the node does already exist */
		m = NODE_FIRST_CHILD(n);
		while(m != NULL) {
			if(strcmp(m->name,SERVICE_CLIENT_ALL) == 0) {
				*child = m;
				return 0;
			}
			m = m->next;
		}

		name = (char*)SERVICE_CLIENT_ALL;
	}
	else {
		/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
		name = kheap_alloc(12 * sizeof(char));
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
		if((n->mode & MODE_SERVICE_SINGLEPIPE) == 0)
			kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	*child = m;
	return 0;
}

static s32 vfsn_createPipe(sVFSNode *n,sVFSNode **child) {
	char *name;
	sVFSNode *m;
	u32 len;
	sProc *p = proc_getRunning();

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	/* we want to have to form <pid>.<x>, therefore two ints and a '.' */
	name = kheap_alloc((11 * 2 + 2) * sizeof(char));
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

static sVFSNode *vfsn_createPipeNode(tPid pid,sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	node->mode = MODE_TYPE_PIPE | MODE_OWNER_READ | MODE_OWNER_WRITE |
		MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->readHandler = vfs_defReadHandler;
	return node;
}

static sVFSNode *vfsn_requestNode(void) {
	sVFSNode *node;
	if(freeList == NULL)
		util_panic("No free vfs-nodes!");

	node = freeList;
	freeList = freeList->next;
	return node;
}

static void vfsn_releaseNode(sVFSNode *node) {
	vassert(node != NULL,"node == NULL");
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
		vid_printf("\tfirstChild: 0x%x\n",node->firstChild);
		vid_printf("\tlastChild: 0x%x\n",node->lastChild);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\tprev: 0x%x\n",node->prev);
		vid_printf("\towner: %d\n",node->owner);
		if(node->mode & MODE_TYPE_SERVUSE) {
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
