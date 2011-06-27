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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/file.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/pmem.h>
#include <sys/mem/dynarray.h>
#include <sys/task/env.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>
#include <assert.h>
#include <errors.h>
#include <ctype.h>

/**
 * Appends the given node as last child to the parent
 */
static void vfs_node_appendChild(sVFSNode *parent,sVFSNode *node);
/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 */
static sVFSNode *vfs_node_requestNode(void);
/**
 * Releases the given node
 */
static void vfs_node_releaseNode(sVFSNode *node);
static void vfs_node_dbg_doPrintTree(size_t level,sVFSNode *parent);

/* all nodes (expand dynamically) */
static sDynArray nodeArray;
/* a pointer to the first free node (which points to the next and so on) */
static sVFSNode *freeList = NULL;
static uint nextUsageId = 0;

void vfs_node_init(void) {
	dyna_start(&nodeArray,sizeof(sVFSNode),VFSNODE_AREA,VFSNODE_AREA_SIZE);
}

bool vfs_node_isValid(inode_t nodeNo) {
	return nodeNo >= 0 && nodeNo < (inode_t)nodeArray.objCount;
}

inode_t vfs_node_getNo(const sVFSNode *node) {
	return (inode_t)dyna_getIndex(&nodeArray,node);
}

sVFSNode *vfs_node_get(inode_t nodeNo) {
	return (sVFSNode*)dyna_getObj(&nodeArray,nodeNo);
}

sVFSNode *vfs_node_getFirstChild(const sVFSNode *node) {
	if(!MODE_IS_LINK((node)->mode))
		return node->firstChild;
	return vfs_link_resolve(node)->firstChild;
}

int vfs_node_getInfo(inode_t nodeNo,sFileInfo *info) {
	sVFSNode *n = vfs_node_get(nodeNo);

	if(n->mode == 0)
		return ERR_INVALID_INODENO;

	/* some infos are not available here */
	/* TODO needs to be completed */
	info->device = VFS_DEV_NO;
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
	info->size = 0;
	return 0;
}

int vfs_node_chmod(inode_t nodeNo,mode_t mode) {
	sVFSNode *n = vfs_node_get(nodeNo);
	/* TODO */
	n->mode = mode;
	return 0;
}

int vfs_node_chown(inode_t nodeNo,uid_t uid,gid_t gid) {
	sVFSNode *n = vfs_node_get(nodeNo);
	/* TODO */
	return 0;
}

char *vfs_node_getPath(inode_t nodeNo) {
	static char path[MAX_PATH_LEN];
	size_t nlen,len = 0,total = 0;
	sVFSNode *node = vfs_node_get(nodeNo);
	sVFSNode *n = node;

	assert(vfs_node_isValid(nodeNo));
	/* the root-node of the whole vfs has no path */
	vassert(n->parent != NULL,"node->parent == NULL");

	while(n->parent != NULL) {
		/* name + slash */
		total += n->nameLen + 1;
		n = n->parent;
	}

	/* not nice, but ensures that we don't overwrite something :) */
	if(total > MAX_PATH_LEN)
		total = MAX_PATH_LEN;

	n = node;
	len = total;
	while(n->parent != NULL) {
		nlen = n->nameLen + 1;
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

char *vfs_node_absolutize(char *dst,size_t size,const char *src) {
	if(*src != '/') {
		size_t len;
		sProc *p = proc_getRunning();
		const char *cwd = env_get(p->pid,"CWD");
		if(cwd) {
			strncpy(dst,cwd,size);
			dst[size - 1] = '\0';
			len = strlen(dst);
			if(len < size - 1 && dst[len - 1] != '/') {
				dst[len++] = '/';
				dst[len] = '\0';
			}
		}
		else {
			/* assume '/' */
			len = 1;
			dst[0] = '/';
			dst[1] = '\0';
		}
		strncpy(dst + len,src,size - len);
		dst[size - 1] = '\0';
		return dst;
	}
	return (char*)src;
}

int vfs_node_resolvePath(const char *path,inode_t *nodeNo,bool *created,uint flags) {
	static char apath[MAX_PATH_LEN];
	sVFSNode *dir,*n = vfs_node_get(0);
	sThread *t = thread_getRunning();
	int pos = 0,depth,lastdepth;
	if(created)
		*created = false;

	/* no absolute path? */
	if(*path != '/') {
		vfs_node_absolutize(apath,sizeof(apath),path);
		path = apath;
	}

	/* skip slashes */
	while(*path == '/')
		path++;

	/* root/current node requested? */
	if(!*path) {
		*nodeNo = vfs_node_getNo(n);
		return 0;
	}

	depth = 0;
	lastdepth = -1;
	dir = n;
	n = vfs_node_getFirstChild(n);
	while(n != NULL) {
		/* go to next '/' and check for invalid chars */
		if(depth != lastdepth) {
			char c;
			pos = 0;
			while((c = path[pos]) && c != '/') {
				if((c != ' ' && isspace(c)) || !isprint(c))
					return ERR_INVALID_PATH;
				pos++;
			}
			lastdepth = depth;
		}

		if((int)n->nameLen == pos && strncmp(n->name,path,pos) == 0) {
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

			if(IS_DRIVER(n->mode))
				break;

			/* move to childs of this node */
			dir = n;
			n = vfs_node_getFirstChild(n);
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
			size_t nameLen;
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
			/* copy the name because vfs_file_create() will store the pointer */
			nameCpy = cache_alloc(nameLen + 1);
			if(nameCpy == NULL)
				return ERR_NOT_ENOUGH_MEM;
			memcpy(nameCpy,path,nameLen + 1);
			/* now create the node and pass the node-number back */
			if((child = vfs_file_create(t->tid,dir,nameCpy,vfs_file_read,vfs_file_write)) == NULL) {
				cache_free(nameCpy);
				return ERR_NOT_ENOUGH_MEM;
			}
			if(created)
				*created = true;
			*nodeNo = vfs_node_getNo(child);
			return 0;
		}
		return ERR_PATH_NOT_FOUND;
	}

	/* resolve link */
	if(!(flags & VFS_NOLINKRES) && MODE_IS_LINK(n->mode))
		n = vfs_link_resolve(n);

	/* virtual node */
	*nodeNo = vfs_node_getNo(n);
	return 0;
}

char *vfs_node_basename(char *path,size_t *len) {
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

void vfs_node_dirname(char *path,size_t len) {
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

sVFSNode *vfs_node_findInDir(const sVFSNode *node,const char *name,size_t nameLen) {
	sVFSNode *n = vfs_node_getFirstChild(node);
	while(n != NULL) {
		if(n->nameLen == nameLen && strncmp(n->name,name,nameLen) == 0)
			return n;
		n = n->next;
	}
	return NULL;
}

sVFSNode *vfs_node_create(sVFSNode *parent,char *name) {
	sVFSNode *node;
	vassert(name != NULL,"name == NULL");

	if(strlen(name) > MAX_NAME_LEN)
		return NULL;

	node = vfs_node_requestNode();
	if(node == NULL)
		return NULL;

	/* ensure that all values are initialized properly */
	node->name = name;
	node->nameLen = strlen(name);
	node->mode = 0;
	node->owner = INVALID_PID;
	node->refCount = 0;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	vfs_node_appendChild(parent,node);
	return node;
}

void vfs_node_destroy(sVFSNode *n) {
	/* remove childs */
	sVFSNode *tn;
	sVFSNode *child = n->firstChild;
	while(child != NULL) {
		tn = child->next;
		vfs_node_destroy(child);
		child = tn;
	}

	/* let the node clean up */
	if(n->destroy)
		n->destroy(n);

	/* free name */
	if(IS_ON_HEAP(n->name))
		cache_free(n->name);

	/* remove from parent and release */
	if(n->prev != NULL)
		n->prev->next = n->next;
	else
		n->parent->firstChild = n->next;
	if(n->next != NULL)
		n->next->prev = n->prev;
	else
		n->parent->lastChild = n->prev;
	vfs_node_releaseNode(n);
}

char *vfs_node_getId(pid_t pid) {
	char *name;
	size_t len,size;

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign = 11 bytes */
	/* we want to have to form <pid>.<x>, therefore two ints, a '.' and \0 */
	/* TODO this is dangerous, because sizeof(int) might be not 4 */
	size = 11 * 2 + 1 + 1;
	name = (char*)cache_alloc(size);
	if(name == NULL)
		return NULL;

	/* create usage-node */
	itoa(name,size,pid);
	len = strlen(name);
	*(name + len) = '.';
	itoa(name + len + 1,size - (len + 1),nextUsageId++);
	return name;
}

void vfs_node_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfs_node_dbg_doPrintTree(1,vfs_node_get(0));
}

void vfs_node_printNode(const sVFSNode *node) {
	vid_printf("VFSNode @ %p:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name ? node->name : "NULL");
		vid_printf("\tfirstChild: %p\n",node->firstChild);
		vid_printf("\tlastChild: %p\n",node->lastChild);
		vid_printf("\tnext: %p\n",node->next);
		vid_printf("\tprev: %p\n",node->prev);
		vid_printf("\towner: %d\n",node->owner);
	}
}

static void vfs_node_appendChild(sVFSNode *parent,sVFSNode *node) {
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

static sVFSNode *vfs_node_requestNode(void) {
	sVFSNode *node = NULL;
	if(freeList == NULL) {
		size_t i,oldCount = nodeArray.objCount;
		if(!dyna_extend(&nodeArray))
			util_panic("No free VFS-nodes");
		freeList = vfs_node_get(oldCount);
		for(i = oldCount; i < nodeArray.objCount - 1; i++) {
			node = vfs_node_get(i);
			node->next = vfs_node_get(i + 1);
		}
		node->next = NULL;
	}

	node = freeList;
	freeList = freeList->next;
	return node;
}

static void vfs_node_releaseNode(sVFSNode *node) {
	vassert(node != NULL,"node == NULL");
	/* mark unused */
	node->name = NULL;
	node->nameLen = 0;
	node->owner = INVALID_PID;
	node->next = freeList;
	freeList = node;
}

static void vfs_node_dbg_doPrintTree(size_t level,sVFSNode *parent) {
	size_t i;
	sVFSNode *n = vfs_node_getFirstChild(parent);
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		/* don't recurse for "." and ".." */
		if(strncmp(n->name,".",1) != 0 && strncmp(n->name,"..",2) != 0)
			vfs_node_dbg_doPrintTree(level + 1,n);
		n = n->next;
	}
}
