/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <vfs/file.h>
#include <vfs/dir.h>
#include <vfs/link.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/openfile.h>
#include <mem/pagedir.h>
#include <mem/cache.h>
#include <mem/physmem.h>
#include <mem/dynarray.h>
#include <task/groups.h>
#include <task/proc.h>
#include <util.h>
#include <video.h>
#include <boot.h>
#include <cppsupport.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

/* we have to use the max-size of all VFSNode sub-classes. that's unfortunate, but I don't see a
 * better way */
static size_t getMaxSize() {
	return MAX(sizeof(VFSChannel),
			MAX(sizeof(VFSDevice),
			MAX(sizeof(VFSDir),
			MAX(sizeof(VFSFile),
			MAX(sizeof(VFSMS),sizeof(VFSLink))))));
}

/* all nodes (expand dynamically) */
DynArray VFSNode::nodeArray(getMaxSize(),VFSNODE_AREA,VFSNODE_AREA_SIZE);
/* a pointer to the first free node (which points to the next and so on) */
VFSNode *VFSNode::freeList = NULL;
uint VFSNode::nextUsageId = 0;
SpinLock VFSNode::nodesLock;
SpinLock VFSNode::treeLock;
size_t VFSNode::allocated;

/* we have 2 refs at the beginning because we expect the creator to release the node if he's done
 * working with it */
VFSNode::VFSNode(pid_t pid,char *n,uint m,bool &success)
		: lock(), name(n), nameLen(), refCount(2), owner(pid), uid(), gid(), mode(m),
		  parent(), prev(), firstChild(), next() {
	if(this == nullptr || name == NULL || nameLen > NAME_MAX) {
		success = false;
		return;
	}

	nameLen = strlen(name);
	const Proc *p = pid != INVALID_PID ? Proc::getRef(pid) : NULL;
	if(p) {
		uid = p->getEUid();
		gid = p->getEGid();
		Proc::relRef(p);
	}
	else {
		uid = ROOT_UID;
		gid = ROOT_GID;
	}

	modtime = acctime = crttime = Timer::getTime();
}

bool VFSNode::isDeletable() const {
	return getOwner() != KERNEL_PID;
}

const VFSNode *VFSNode::openDir(bool locked,bool *valid) const {
	const VFSNode *p;
	if(!S_ISLNK(mode))
		p = this;
	else
		p = static_cast<const VFSLink*>(this)->resolve();
	if(locked)
		treeLock.down();
	*valid = p->name != NULL;
	return p->firstChild;
}

int VFSNode::isEmptyDir() const {
	if(!S_ISDIR(mode))
		return -ENOTDIR;
	bool valid;
	const VFSNode *c = openDir(true,&valid);
	if(valid) {
		bool res = c->next && !c->next->next;
		closeDir(true);
		return res ? 0 : -ENOTEMPTY;
	}
	closeDir(true);
	return -EDESTROYED;
}

void VFSNode::getInfo(pid_t pid,struct stat *info) {
	info->st_dev = VFS_DEV_NO;
	info->st_atime = acctime;
	info->st_mtime = modtime;
	info->st_ctime = crttime;
	info->st_ino = getNo();
	// TODO set that properly
	info->st_nlink = 1;
	info->st_uid = uid;
	info->st_gid = gid;
	info->st_mode = mode;
	info->st_size = getSize(pid);
	info->st_blksize = 512;
	info->st_blocks = info->st_size / 512;
}

void VFSNode::getPathTo(char *dst,size_t size) const {
	size_t nlen,len = 0,total = 0;
	const VFSNode *n = this;
	if(!n->isAlive()) {
		strnzcpy(dst,"<destroyed>",size);
		return;
	}

	while(n->parent != NULL) {
		/* name + slash */
		total += n->nameLen + 1;
		n = n->parent;
	}

	/* not nice, but ensures that we don't overwrite something :) */
	if(total > size)
		total = size;

	n = this;
	len = total;
	while(n->parent != NULL) {
		nlen = n->nameLen + 1;
		/* insert the new element */
		*(dst + total - nlen) = '/';
		memcpy(dst + total + 1 - nlen,n->name,nlen - 1);
		total -= nlen;
		n = n->parent;
	}

	/* terminate */
	*(dst + len) = '\0';
}

int VFSNode::chmod(pid_t pid,mode_t m) {
	int res = 0;
	const Proc *p = pid == KERNEL_PID ? NULL : Proc::getRef(pid);
	/* root can chmod everything; others can only chmod their own files */
	if(p) {
		if(p->getEUid() != uid && p->getEUid() != ROOT_UID)
			res = -EPERM;
		Proc::relRef(p);
	}
	if(res == 0)
		mode = (mode & ~MODE_PERM) | (m & MODE_PERM);
	return res;
}

int VFSNode::chown(pid_t pid,uid_t nuid,gid_t ngid) {
	int res = 0;
	const Proc *p = pid == KERNEL_PID ? NULL : Proc::getRef(pid);
	if(p) {
		/* root can chown everything; others can only chown their own files */
		if(p->getEUid() != uid && p->getEUid() != ROOT_UID)
			res = -EPERM;
		else if(p->getEUid() != ROOT_UID) {
			/* users can't change the owner */
			if(nuid != (uid_t)-1 && nuid != uid && nuid != p->getEUid())
				res = -EPERM;
			/* users can change the group only to a group they're a member of */
			else if(ngid != (gid_t)-1 && ngid != gid && ngid != p->getEGid() &&
					!Groups::contains(p->getPid(),ngid))
				res = -EPERM;
		}
		Proc::relRef(p);
	}

	if(res == 0) {
		if(nuid != (uid_t)-1)
			uid = nuid;
		if(ngid != (gid_t)-1)
			gid = ngid;
	}
	return res;
}

int VFSNode::utime(pid_t pid,const struct utimbuf *utimes) {
	int res = 0;
	const Proc *p = pid == KERNEL_PID ? NULL : Proc::getRef(pid);
	if(p) {
		if(p->getEUid() != uid && p->getEUid() != ROOT_UID)
			res = -EPERM;
		Proc::relRef(p);
	}

	if(res == 0) {
		modtime = utimes->modtime;
		acctime = utimes->actime;
	}
	return res;
}

int VFSNode::request(const char *path,const char **end,VFSNode **node,bool *created,
		uint flags,mode_t mode) {
	const VFSNode *dir,*n = *node;
	const Thread *t = Thread::getRunning();
	/* at the beginning, t might be NULL */
	pid_t pid = t ? t->getProc()->getPid() : KERNEL_PID;
	int pos = 0,err,depth,lastdepth;
	bool valid;
	if(created)
		*created = false;
	if(n == NULL)
		n = get(0);
	*node = NULL;

	/* skip slashes */
	while(*path == '/')
		path++;

	/* root/current node requested? */
	if(!*path) {
		*node = const_cast<VFSNode*>(n->increaseRefs());
		err = *node == NULL ? -ENOENT : 0;
		return err;
	}

	depth = 0;
	lastdepth = -1;
	dir = n;
	n = dir->openDir(true,&valid);
	if(valid) {
		while(n != NULL) {
			/* go to next '/' and check for invalid chars */
			if(depth != lastdepth) {
				char c;
				/* check if we can access this directory */
				if((err = VFS::hasAccess(pid,dir,VFS_EXEC)) < 0)
					goto done;

				pos = 0;
				while((c = path[pos]) && c != '/') {
					if((c != ' ' && isspace(c)) || !isprint(c)) {
						err = -EINVAL;
						goto done;
					}
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
				while(*path == '/')
					path++;
				/* "/" at the end is optional */
				if(!*path)
					break;

				if(IS_DEVICE(n->mode))
					break;

				/* move to childs of this node */
				dir = n;
				n = dir->openDir(false,&valid);
				if(!valid) {
					err = -EDESTROYED;
					goto done;
				}
				depth++;
				continue;
			}
			n = n->next;
		}
	}

	err = 0;
	if(n == NULL) {
		dir->closeDir(true);
		/* should we create a default-file? */
		if((flags & VFS_CREATE) && S_ISDIR(dir->mode))
			err = createFile(pid,path,const_cast<VFSNode*>(dir),node,created,mode);
		else
			err = -ENOENT;
	}
	else {
		if(flags & VFS_EXCL) {
			err = -EEXIST;
			goto done;
		}

		/* resolve link */
		if(!(flags & VFS_NOLINKRES) && S_ISLNK(n->mode))
			n = const_cast<VFSNode*>(static_cast<const VFSLink*>(n)->resolve());

		/* virtual node */
		*node = const_cast<VFSNode*>(n->increaseRefs());
		if(*node == NULL)
			err = -ENOENT;
		dir->closeDir(true);
	}
	if(end)
		*end = path;
	return err;

done:
	dir->closeDir(true);
	return err;
}

char *VFSNode::basename(char *path,size_t *len) {
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

void VFSNode::dirname(char *path,size_t len) {
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
	while(len > 0 && *p != '/') {
		p--;
		len--;
	}

	/* set new end */
	*(p + 1) = '\0';
}

const VFSNode *VFSNode::findInDir(const char *ename,size_t enameLen) const {
	bool valid = false;
	const VFSNode *res = NULL;
	const VFSNode *n = openDir(true,&valid);
	if(valid) {
		while(n != NULL) {
			if(n->nameLen == enameLen && strncmp(n->name,ename,enameLen) == 0) {
				res = n;
				break;
			}
			n = n->next;
		}
	}
	closeDir(true);
	return res;
}

void VFSNode::append(VFSNode *p) {
	if(p != NULL) {
		{
			LockGuard<SpinLock> g(&treeLock);
			prev = NULL;
			next = p->firstChild;
			if(next)
				next->prev = this;
			p->firstChild = this;
		}

		LockGuard<SpinLock> g(&p->lock);
		p->refCount++;
	}
	parent = p;
}

ushort VFSNode::doUnref(bool remove) {
	const char *nameptr = NULL;
	/* first check whether we have the last ref */
	ushort remRefs = refCount;
	bool norefs = remRefs == 1;

	/* no remove the child-nodes, if necessary. this is done unlocked which also means that */
	if(norefs || remove) {
		VFSNode *child = firstChild;
		while(child != NULL) {
			VFSNode *tn = child->next;
			/* if the parent is destroyed, the childs have to be destroyed as well, regardless of
			 * whether they have still references or not (of course, the node isn't free'd if there
			 * are still references) */
			child->doUnref(true);
			child = tn;
		}
	}

	{
		LockGuard<SpinLock> g1(&treeLock);
		/* don't decrease the refs twice with remove */
		if(!remove || name) {
			LockGuard<SpinLock> g2(&lock);
			remRefs = --refCount;
			norefs = remRefs == 0;
		}

		/* take care that we don't destroy the node twice */
		if(norefs)
			invalidate();
		if((norefs || remove) && name) {
			/* remove from parent and release (attention: maybe its not yet in the tree) */
			if(prev)
				prev->next = next;
			else if(parent)
				parent->firstChild = next;
			if(next)
				next->prev = prev;

			prev = NULL;
			next = NULL;
			firstChild = NULL;

			/* free name (do that afterwards, unlocked) */
			if(IS_ON_HEAP(name))
				nameptr = name;
			name = NULL;
		}
	}

	if(nameptr)
		Cache::free(const_cast<char*>(nameptr));
	/* if there are no references anymore, we can put the node on the freelist */
	if(norefs) {
		parent->unref();
		delete this;
	}
	return remRefs;
}

char *VFSNode::generateId(pid_t pid) {
	/* we want a id in the form <pid>.<x>, i.e. 2 ints, a '.' and '\0'. thus, allowing up to 31
	 * digits per int is enough, even for 64-bit ints */
	const size_t size = 64;
	char *name = (char*)Cache::alloc(size);
	if(name == NULL)
		return NULL;

	/* create usage-node */
	itoa(name,size,pid);
	size_t len = strlen(name);
	*(name + len) = '.';
	uint id;
	{
		LockGuard<SpinLock> g(&nodesLock);
		id = nextUsageId++;
	}
	itoa(name + len + 1,size - (len + 1),id);
	return name;
}

int VFSNode::createFile(pid_t pid,const char *path,VFSNode *dir,VFSNode **child,bool *created,mode_t mode) {
	size_t nameLen;
	int err;
	/* can we create files in this directory? */
	if((err = VFS::hasAccess(pid,dir,VFS_WRITE)) < 0)
		return err;

	char *nextSlash = strchr(path,'/');
	if(nextSlash) {
		/* if there is still a slash in the path, we can't create the file */
		if(*(nextSlash + 1) != '\0')
			return -EINVAL;
		*nextSlash = '\0';
		nameLen = nextSlash - path;
	}
	else
		nameLen = strlen(path);
	/* copy the name because vfs_file_create() will store the pointer */
	char *nameCpy = (char*)Cache::alloc(nameLen + 1);
	if(nameCpy == NULL)
		return -ENOMEM;
	memcpy(nameCpy,path,nameLen + 1);

	/* now create the node and pass the node-number back */
	*child = createObj<VFSFile>(pid,dir,nameCpy,mode);
	if(*child == NULL) {
		Cache::free(nameCpy);
		return -ENOMEM;
	}
	if(created)
		*created = true;
	return 0;
}

void *VFSNode::operator new(size_t) throw() {
	LockGuard<SpinLock> g(&nodesLock);
	VFSNode *node = NULL;
	if(freeList == NULL) {
		size_t oldCount = nodeArray.getObjCount();
		if(!nodeArray.extend())
			return NULL;
		freeList = get(oldCount);
		for(size_t i = oldCount; i < nodeArray.getObjCount() - 1; i++) {
			node = get(i);
			node->next = get(i + 1);
		}
		node->next = NULL;
	}

	node = freeList;
	freeList = freeList->next;
	allocated++;
	return node;
}

void VFSNode::operator delete(void *ptr) throw() {
	VFSNode *node = static_cast<VFSNode*>(ptr);
	vassert(node != NULL,"node == NULL");
	/* mark unused */
	node->name = NULL;
	node->nameLen = 0;
	node->owner = INVALID_PID;
	LockGuard<SpinLock> g(&nodesLock);
	node->next = freeList;
	freeList = node;
	allocated--;
}

void VFSNode::printTree(OStream &os) {
	os.writef("VFS:\n");
	os.writef("/\n");
	LockGuard<SpinLock> g(&treeLock);
	doPrintTree(os,1,get(0));
}

void VFSNode::print(OStream &os) const {
	os.writef("VFSNode @ %p:\n",this);
	if(this) {
		os.writef("\tname: %s\n",name ? name : "NULL");
		os.writef("\tfirstChild: %p\n",firstChild);
		os.writef("\tnext: %p\n",next);
		os.writef("\tprev: %p\n",prev);
		os.writef("\towner: %d\n",owner);
	}
}

void VFSNode::doPrintTree(OStream &os,size_t level,const VFSNode *parent) {
	bool valid;
	const VFSNode *n = parent->openDir(false,&valid);
	if(valid) {
		while(n != NULL) {
			for(size_t i = 0;i < level;i++)
				os.writef(" |");
			os.writef("- %s\n",n->name);
			/* don't recurse for "." and ".." */
			if(strncmp(n->name,".",1) != 0 && strncmp(n->name,"..",2) != 0)
				doPrintTree(os,level + 1,n);
			n = n->next;
		}
	}
	parent->closeDir(false);
}
