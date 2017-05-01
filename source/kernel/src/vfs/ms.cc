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

#include <task/proc.h>
#include <vfs/info.h>
#include <vfs/ms.h>
#include <common.h>
#include <ostringstream.h>
#include <util.h>

VFSMS::MSTreeItem::MSTreeItem(const VFSMS::MSTreeItem &i)
		: esc::PathTreeItem<OpenFile>(i), root(i.root), devno(i.devno) {
	if(getData())
		getData()->incRefs();
}

VFSMS::VFSMS(pid_t pid,VFSNode *parent,char *name,uint mode,bool &success)
	: VFSDir(pid,parent,name,MODE_TYPE_MOUNTSPC | mode,success), _lock(), _tree() {
	if(!success)
		return;

	success = init();
}

VFSMS::VFSMS(pid_t pid,const VFSMS &ms,VFSNode *parent,char *name,uint mode,bool &success)
	: VFSDir(pid,parent,name,MODE_TYPE_MOUNTSPC | mode,success), _lock(), _tree() {
	if(!success)
		return;
	{
		LockGuard<SpinLock> guard(&ms._lock);
		if(_tree.replaceWith(ms._tree) != 0) {
			success = false;
			return;
		}
	}

	success = init();
}

bool VFSMS::init() {
	VFSNode *info = createObj<VFSInfo::MountsFile>(KERNEL_PID,this);
	if(info == NULL)
		return false;
	VFSNode::release(info);

	/* auto-destroy if the last process stops using it */
	refCount--;
	return true;
}

ino_t VFSMS::request(const char *path,const char **end,OpenFile **file) {
	ino_t res = 0;
	{
		LockGuard<SpinLock> guard(&_lock);
		MSTreeItem *match = _tree.find(path,end);
		if(!match)
			return -ENOENT;
		*file = match->getData();
		res = match->root;
	}
	(*file)->incUsages();
	return res;
}

void VFSMS::release(OpenFile *file) {
	file->decUsages();
}

int VFSMS::mount(Proc *,const char *path,OpenFile *file) {
	file->incRefs();
	LockGuard<SpinLock> guard(&_lock);
	return _tree.insert(path,file);
}

int VFSMS::remount(Proc *p,const char *path,OpenFile *dir,uint flags) {
	LockGuard<SpinLock> guard(&_lock);
	const char *end;
	MSTreeItem *match = _tree.find(path,&end);
	if(!match)
		return -ENOENT;

	/* create a new file, if necessary with adjusted permissions */
	OpenFile *nfile;
	OpenFile *old = match->getData();
	const uint rwx = VFS_READ | VFS_WRITE | VFS_EXEC;
	if((flags & rwx) != (old->getFlags() & rwx)) {
		flags = (old->getFlags() & ~rwx) | (flags & rwx);
		int res = OpenFile::getFree(p->getPid(),flags,flags,old->getNodeNo(),old->getDev(),
			old->getNode(),&nfile,true);
		if(res < 0)
			return res;
	}
	else {
		nfile = old;
		nfile->incRefs();
	}

	/* if we remount at the same place, remove the old one */
	if(end[0] == '\0') {
		_tree.remove(path);
		old->close(p->getPid());
	}

	/* mount it */
	int res = _tree.insert(path,nfile);
	if(res < 0) {
		nfile->close(p->getPid());
		return res;
	}

	/* remember root inode */
	match = _tree.find(path,&end);
	assert(match != NULL);
	match->root = dir->getNodeNo();
	match->devno = dir->getDev();
	/* increase references to prevent that we delete the node while it's mounted */
	/* but prevent circular dependencies (otherwise, we don't free the mountspace) */
	if(match->devno == VFS_DEV_NO && dir->getNode() != this)
		VFSNode::request(match->root);
	return 0;
}

int VFSMS::unmount(Proc *p,const char *path) {
	OpenFile *file;
	{
		LockGuard<SpinLock> guard(&_lock);
		/* release vfs node for moints of virtual filesystems */
		const char *end;
		MSTreeItem *match = _tree.find(path,&end);
		if(!match)
			return -ENOENT;
		/* unmounting of directories is not allowed */
		/* TODO this could be extended to allow it if it reduces permissions */
		if(match->root != 0)
			return -EPERM;
		if(match->devno == VFS_DEV_NO) {
			VFSNode *node = VFSNode::get(match->root);
			if(node != this)
				VFSNode::release(node);
		}
		/* remove the mountpoint */
		file = _tree.remove(path);
		assert(file != NULL);
	}

	file->close(p->getPid());
	return 0;
}

void VFSMS::join(Proc *p) {
	if(p->msnode)
		p->msnode->leave(p);
	ref();
	p->msnode = this;
}

void VFSMS::leave(Proc *p) {
	if(unref() == 1)
		destroy();
	p->msnode = NULL;
}

void VFSMS::readCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	OStringStream os;
	node->print(os);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSMS::print(OStream &os) const {
	LockGuard<SpinLock> guard(&_lock);
	os.writef("Mountspace %s:\n",getName());
	_tree.print(os,printItem);
}

void VFSMS::printItem(OStream &os,MSTreeItem *item) {
	OpenFile *file = item->getData();
	os.writef("%s [flags=%#x root=%d]",file->getPath(),file->getFlags(),item->root);
}
