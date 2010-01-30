/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/info.h>
#include <vfs/driver.h>
#include <vfs/rw.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <mem/pmem.h>
#include <util.h>
#include <video.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

#define IS_ON_HEAP(addr) ((u32)(addr) >= KERNEL_HEAP_START && \
		(u32)(addr) < KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/**
 * Creates a pipe-node for given thread
 *
 * @param tid the thread-id
 * @param parent the parent-node
 * @param name the node-name
 * @return the node or NULL
 */
static sVFSNode *vfsn_createPipeNode(tTid tid,sVFSNode *parent,char *name);

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

#if DEBUGGING
/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfsn_dbg_doPrintTree(u32 level,sVFSNode *parent);
#endif


/* all nodes */
sVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static sVFSNode *freeList;
static u32 nextPipeId = 0;

void vfsn_init(void) {
	tInodeNo i;
	sVFSNode *node = &nodes[0];
	freeList = node;
	for(i = 0; i < NODE_COUNT - 1; i++) {
		node->next = node + 1;
		node++;
	}
	node->next = NULL;
}

bool vfsn_isValidNodeNo(tInodeNo nodeNo) {
	return nodeNo >= 0 && nodeNo < NODE_COUNT;
}

bool vfsn_isOwnServiceNode(tInodeNo nodeNo) {
	sThread *t = thread_getRunning();
	sVFSNode *node = nodes + nodeNo;
	return node->owner == t->tid && (node->mode & MODE_TYPE_SERVICE);
}

tInodeNo vfsn_getNodeNo(sVFSNode *node) {
	return NADDR_TO_VNNO(node);
}

sVFSNode *vfsn_getNode(tInodeNo nodeNo) {
	return nodes + nodeNo;
}

s32 vfsn_getNodeInfo(tInodeNo nodeNo,sFileInfo *info) {
	sVFSNode *n = nodes + nodeNo;

	if(n->mode == 0)
		return ERR_INVALID_INODENO;

	/* some infos are not available here */
	/* TODO needs to be completed */
	info->device = VFS_DEV_NO;
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

char *vfsn_getPath(tInodeNo nodeNo) {
	static char path[MAX_PATH_LEN];
	u32 nlen,len = 0,total = 0;
	sVFSNode *node = nodes + nodeNo;
	sVFSNode *n = node;

	vassert(node != NULL,"node = NULL");
	/* the root-node of the whole vfs has no path */
	vassert(n->parent != NULL,"node->parent == NULL");

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
		/* insert the new element */
		*(path + total - nlen) = '/';
		memcpy(path + total + 1 - nlen,n->name,nlen - 1);
		total -= nlen;
		n = n->parent;
	}

	/* terminate */
	*(path + len) = '\0';

	return (char*)path;
}

s32 vfsn_resolvePath(const char *path,tInodeNo *nodeNo,bool *created,u16 flags) {
	sVFSNode *dir,*n = nodes;
	sThread *t = thread_getRunning();
	s32 pos,depth = 0;
	if(created)
		*created = false;

	/* no absolute path? */
	if(*path != '/')
		return ERR_INVALID_PATH;

	/* skip slashes */
	while(*path == '/')
		path++;

	/* root/current node requested? */
	if(!*path) {
		*nodeNo = NADDR_TO_VNNO(n);
		return 0;
	}

	pos = strchri(path,'/');
	dir = n;
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
			dir = n;
			n = NODE_FIRST_CHILD(n);
			depth++;
			continue;
		}
		n = n->next;
	}

	/* Note: this means that no one can create (additional) virtual nodes in the root-directory,
	 * which is intended. The existing virtual nodes in the root-directory, of course, hide
	 * possibly existing directory-entries in the real filesystem with the same name. */
	if(n == NULL) {
		/* not existing file/dir in root-directory means that we should ask fs */
		if(depth == 0)
			return ERR_REAL_PATH;

		/* should we create a default-file? */
		if((flags & VFS_CREATE) && (dir->mode & MODE_TYPE_DIR)) {
			u32 nameLen;
			sVFSNode *child;
			char *nameCpy;
			char *nextSlash = strchr(path,'/');
			if(nextSlash) {
				/* if there is still a slash in the path, we can't create the file */
				if(*(nextSlash + 1) != '\0')
					return ERR_INVALID_PATH;
				*nextSlash = '\0';
				nameLen = nextSlash - path;
			}
			else
				nameLen = strlen(path);
			/* copy the name because vfsn_createFile() will store the pointer */
			nameCpy = kheap_alloc(nameLen + 1);
			if(nameCpy == NULL)
				return ERR_NOT_ENOUGH_MEM;
			memcpy(nameCpy,path,nameLen + 1);
			/* now create the node and pass the node-number back */
			if((child = vfsn_createFile(t->tid,dir,nameCpy,vfsrw_readDef,vfsrw_writeDef)) == NULL) {
				kheap_free(nameCpy);
				return ERR_NOT_ENOUGH_MEM;
			}
			if(created)
				*created = true;
			*nodeNo = NADDR_TO_VNNO(child);
			return 0;
		}
		return ERR_PATH_NOT_FOUND;
	}

	/* handle special node-types */
	if((flags & VFS_CONNECT) && (n->mode & MODE_TYPE_SERVICE)) {
		sVFSNode *child;
		/* create service-use */
		s32 err = vfsn_createServiceUse(t->tid,n,&child);
		if(err < 0)
			return err;

		/* set new node as resolved one */
		*nodeNo = NADDR_TO_VNNO(child);
		return 0;
	}

	if(!(flags & VFS_NOLINKRES) && MODE_IS_LINK(n->mode)) {
		/* resolve link */
		n = (sVFSNode*)n->data.def.cache;
	}

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

char *vfsn_basename(char *path,u32 *len) {
	char *p = path + *len - 1;
	while(*p == '/') {
		p--;
		(*len)--;
	}
	*(p + 1) = '\0';
	if((p = strrchr(path,'/')) == NULL)
		return path;
	return p + 1;
}

void vfsn_dirname(char *path,u32 len) {
	char *p = path + len - 1;
	/* remove last '/' */
	while(*p == '/') {
		p--;
		len--;
	}

	/* nothing to remove? */
	if(len == 0)
		return;

	/* remove last path component */
	while(*p != '/')
		p--;

	/* set new end */
	*(p + 1) = '\0';
}

sVFSNode *vfsn_findInDir(sVFSNode *node,const char *name,u32 nameLen) {
	sVFSNode *n = NODE_FIRST_CHILD(node);
	while(n != NULL) {
		if(strlen(n->name) == nameLen && strncmp(n->name,name,nameLen) == 0)
			return n;
		n = n->next;
	}
	return NULL;
}

sVFSNode *vfsn_createNodeAppend(sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNode(name);
	vfsn_appendChild(parent,node);
	return node;
}

sVFSNode *vfsn_createNode(char *name) {
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
	node->owner = INVALID_TID;
	node->refCount = 0;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->data.servuse.recvList = NULL;
	node->data.servuse.sendList = NULL;
	node->data.def.cache = NULL;
	node->data.def.size = 0;
	node->data.def.pos = 0;
	return node;
}

sVFSNode *vfsn_createDir(sVFSNode *parent,char *name) {
	sVFSNode *target;
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	if(vfsn_createLink(node,(char*)".",node) == NULL) {
		vfsn_removeNode(node);
		return NULL;
	}
	/* the root-node has no parent */
	target = parent == NULL ? node : parent;
	if(vfsn_createLink(node,(char*)"..",target) == NULL) {
		vfsn_removeNode(node);
		return NULL;
	}

	node->mode = MODE_TYPE_DIR | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OWNER_EXEC |
		MODE_OTHER_READ | MODE_OTHER_EXEC;
	node->readHandler = vfsinfo_dirReadHandler;
	node->writeHandler = NULL;
	return node;
}

sVFSNode *vfsn_createLink(sVFSNode *node,char *name,sVFSNode *target) {
	sVFSNode *child = vfsn_createNodeAppend(node,name);
	if(child == NULL)
		return NULL;
	child->mode = MODE_TYPE_LINK | MODE_OWNER_READ | MODE_OTHER_READ;
	child->data.def.cache = target;
	return child;
}

sVFSNode *vfsn_createPipeCon(sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->mode = MODE_TYPE_PIPECON | MODE_OWNER_READ | MODE_OWNER_WRITE;
	node->readHandler = NULL;
	node->writeHandler = NULL;
	return node;
}

sVFSNode *vfsn_createFile(tTid tid,sVFSNode *parent,char *name,fRead rwHandler,fWrite wrHandler) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = tid;
	/* TODO we need write-permissions for other because we have no real used-/group-based
	 * permission-system */
	node->mode = MODE_TYPE_FILE | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OTHER_READ
		| MODE_OTHER_WRITE;
	node->readHandler = rwHandler;
	node->writeHandler = wrHandler;
	return node;
}

sVFSNode *vfsn_createServiceNode(tTid tid,sVFSNode *parent,char *name,u32 type) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = tid;
	node->mode = MODE_TYPE_SERVICE | MODE_OWNER_READ | MODE_OTHER_READ | type;
	node->readHandler = NULL;
	node->writeHandler = NULL;
	node->data.service.isEmpty = true;
	node->data.service.lastClient = NULL;
	return node;
}

sVFSNode *vfsn_createServiceUseNode(tTid tid,sVFSNode *parent,char *name,fRead rhdlr,fWrite whdlr) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = tid;
	node->mode = MODE_TYPE_SERVUSE | MODE_OWNER_READ | MODE_OWNER_WRITE |
		MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->readHandler = rhdlr;
	node->writeHandler = whdlr;
	return node;
}

void vfsn_appendChild(sVFSNode *parent,sVFSNode *node) {
	vassert(node != NULL,"node == NULL");

	if(parent != NULL) {
		/* invalid cache of directories */
		if(MODE_IS_DIR(parent->mode) && parent->data.def.cache) {
			kheap_free(parent->data.def.cache);
			parent->data.def.cache = NULL;
			parent->data.def.pos = 0;
			parent->data.def.size = 0;
		}

		if(parent->firstChild == NULL)
			parent->firstChild = node;
		if(parent->lastChild != NULL)
			parent->lastChild->next = node;
		node->prev = parent->lastChild;
		parent->lastChild = node;
	}
	node->parent = parent;
}

void vfsn_removeNode(sVFSNode *n) {
	/* remove childs */
	sVFSNode *tn;
	sVFSNode *child = n->firstChild;
	while(child != NULL) {
		tn = child->next;
		vfsn_removeNode(child);
		child = tn;
	}

	if(n->mode & MODE_TYPE_SERVUSE) {
		/* free send and receive list */
		if(n->data.servuse.recvList != NULL) {
			sll_destroy(n->data.servuse.recvList,true);
			n->data.servuse.recvList = NULL;
		}
		if(n->data.servuse.sendList != NULL) {
			sll_destroy(n->data.servuse.sendList,true);
			n->data.servuse.sendList = NULL;
		}
	}
	else if(n->mode & MODE_TYPE_PIPE) {
		sll_destroy(n->data.pipe.list,true);
		n->data.pipe.list = NULL;
	}
	else if(n->data.def.cache != NULL && IS_ON_HEAP(n->data.def.cache)) {
		kheap_free(n->data.def.cache);
		n->data.def.cache = NULL;
	}

	/* free name */
	if(IS_ON_HEAP(n->name))
		kheap_free(n->name);

	/* remove from parent and release */
	if(n->prev != NULL)
		n->prev->next = n->next;
	else
		n->parent->firstChild = n->next;
	if(n->next != NULL)
		n->next->prev = n->prev;
	else
		n->parent->lastChild = n->prev;

	/* invalidate cache of parent-folder */
	if(n->parent->data.def.cache) {
		kheap_free(n->parent->data.def.cache);
		n->parent->data.def.cache = NULL;
		n->parent->data.def.size = 0;
		n->parent->data.def.pos = 0;
	}
	vfsn_releaseNode(n);
}

s32 vfsn_createServiceUse(tTid tid,sVFSNode *n,sVFSNode **child) {
	char *name;
	sVFSNode *m;
	fRead rhdlr;
	fWrite whdlr;

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	name = (char*)kheap_alloc(12 * sizeof(char));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create usage-node */
	itoa(name,tid);

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

	/* ok, create a service-usage-node */
	rhdlr = (fRead)(IS_DRIVER(n->mode) ? vfsdrv_read : NULL);
	whdlr = (fWrite)(IS_DRIVER(n->mode) ? vfsdrv_write : NULL);
	m = vfsn_createServiceUseNode(tid,n,name,rhdlr,whdlr);
	if(m == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	*child = m;
	return 0;
}

s32 vfsn_createPipe(sVFSNode *n,sVFSNode **child) {
	char *name;
	sVFSNode *m;
	u32 len;
	sThread *t = thread_getRunning();

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	/* we want to have to form <pid>.<x>, therefore two ints and a '.' */
	name = (char*)kheap_alloc((11 * 2 + 2) * sizeof(char));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create usage-node */
	itoa(name,t->tid);
	len = strlen(name);
	*(name + len) = '.';
	itoa(name + len + 1,nextPipeId++);

	/* ok, create a pipe-node */
	m = vfsn_createPipeNode(t->tid,n,name);
	if(m == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	*child = m;
	return 0;
}

static sVFSNode *vfsn_createPipeNode(tTid tid,sVFSNode *parent,char *name) {
	sVFSNode *node = vfsn_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = tid;
	node->mode = MODE_TYPE_PIPE | MODE_OWNER_READ | MODE_OWNER_WRITE |
		MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->readHandler = vfsrw_readPipe;
	node->writeHandler = vfsrw_writePipe;
	node->data.pipe.list = NULL;
	node->data.pipe.total = 0;
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void vfsn_dbg_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfsn_dbg_doPrintTree(1,&nodes[0]);
}

static void vfsn_dbg_doPrintTree(u32 level,sVFSNode *parent) {
	u32 i;
	sVFSNode *n = NODE_FIRST_CHILD(parent);
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		/* don't recurse for "." and ".." */
		if(strncmp(n->name,".",1) != 0 && strncmp(n->name,"..",2) != 0)
			vfsn_dbg_doPrintTree(level + 1,n);
		n = n->next;
	}
}

void vfsn_dbg_printNode(sVFSNode *node) {
	vid_printf("VFSNode @ 0x%x:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name ? node->name : "NULL");
		vid_printf("\tfirstChild: 0x%x\n",node->firstChild);
		vid_printf("\tlastChild: 0x%x\n",node->lastChild);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\tprev: 0x%x\n",node->prev);
		vid_printf("\towner: %d\n",node->owner);
		if(node->mode & MODE_TYPE_SERVUSE) {
			vid_printf("\tSendList:\n");
			sll_dbg_print(node->data.servuse.sendList);
			vid_printf("\tRecvList:\n");
			sll_dbg_print(node->data.servuse.recvList);
		}
		else {
			vid_printf("\treadHandler: 0x%x\n",node->readHandler);
			vid_printf("\twriteHandler: 0x%x\n",node->writeHandler);
			vid_printf("\tcache: 0x%x\n",node->data.def.cache);
			vid_printf("\tsize: %d\n",node->data.def.size);
			vid_printf("\tpos: %d\n",node->data.def.pos);
		}
	}
}

#endif
