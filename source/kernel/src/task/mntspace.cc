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

#include <task/mntspace.h>
#include <task/proc.h>
#include <common.h>
#include <ostringstream.h>
#include <util.h>

static void printPath(OStream &os,MSTreeItem *item) {
	if(item->getParent() != item) {
		printPath(os,static_cast<MSTreeItem*>(item->getParent()));
	}
	os.writef("%s",item->getName());
	if(!(item->getName()[0] == '/' && item->getName()[1] == '\0'))
		os.writec('/');
}

static const char *decodeFlags(uint flags) {
	static char buf[4];
	buf[0] = (flags & VFS_READ) ? 'r' : '-';
	buf[1] = (flags & VFS_WRITE) ? 'w' : '-';
	buf[2] = (flags & VFS_EXEC) ? 'x' : '-';
	buf[3] = '\0';
	return buf;
}

void MSPathTree::printRec(OStream &os,MSTreeItem *item) const {
	if(!item)
		return;

	OpenFile *file = item->getData();
	if(file) {
		// do not print the path to the channel, but to the device
		if(IS_CHANNEL(file->getNode()->getMode()))
			os.writef("%s on ",file->getNode()->getParent()->getPath());
		else
			os.writef("%s on ",file->getPath());
		printPath(os,item);
		os.writef(" type %s (%s",
			IS_CHANNEL(file->getNode()->getMode()) ? "user" : "kernel",
			decodeFlags(file->getFlags()));
		if(item->root)
			os.writef(",rootino=%d",item->root);
		os.writef(")\n");
	}

	MSTreeItem *n = static_cast<MSTreeItem*>(item->_child);
	while(n) {
		printRec(os,n);
		n = static_cast<MSTreeItem*>(n->_next);
	}
}

MSTreeItem::MSTreeItem(const MSTreeItem &i)
		: esc::PathTreeItem<OpenFile>(i), root(i.root), devno(i.devno) {
	if(getData())
		getData()->incRefs();
}

// we use steadily increasing 64 bit ids to make them "unique"
MntSpace::id_t MntSpace::_nextId = 0;
// however, we only need a couple of mountspaces, so that a static table is enough. The mountspaces
// are reference counted, denoting the number of processes using a mountspace. The idea is to give
// the process a reference to its mountspace, so that we can use a pointer to make it efficient
// (opening files). The associated VFS nodes use the 64 bit id and need to request a reference to
// access the mountspace. This way, we can have open files for the VFS nodes even if we already
// destroyed the associated mountspace. The VFS nodes will still exist, because OpenFile keeps a
// reference, but they will not be alive anymore (the mountspace destructor destroys its VFS
// subtree).
SpinLock MntSpace::_tableLock;
MntSpace *MntSpace::_table[MAX_MNT_SPACES];

MntSpace::MntSpace(id_t id,pid_t pid,VFSNode *parent,char *name)
	: _id(id), _refs(), _lock(), _tree(), _node(createObj<VFSMS>(pid,parent,id,name,0700)) {
}

MntSpace::~MntSpace() {
	// TODO it would be better to move sub mountspaces to our parent
	_node->destroy();
}

MntSpace *MntSpace::create(pid_t pid,VFSNode *parent,char *name) {
	LockGuard<SpinLock> guard(&_tableLock);
	for(size_t i = 0; i < MAX_MNT_SPACES; ++i) {
		if(_table[i] == NULL) {
			_table[i] = new MntSpace(_nextId++,pid,parent,name);
			return _table[i];
		}
	}
	return NULL;
}

MntSpace *MntSpace::request(id_t id) {
	LockGuard<SpinLock> guard(&_tableLock);
	for(size_t i = 0; i < MAX_MNT_SPACES; ++i) {
		if(_table[i] && _table[i]->_id == id) {
			_table[i]->_refs++;
			return _table[i];
		}
	}
	return NULL;
}

void MntSpace::release(MntSpace *ms) {
	LockGuard<SpinLock> guard(&_tableLock);
	if(--ms->_refs == 0) {
		for(size_t i = 0; i < MAX_MNT_SPACES; ++i) {
			if(_table[i] == ms) {
				_table[i] = NULL;
				break;
			}
		}
		delete ms;
	}
}

MntSpace *MntSpace::clone(pid_t pid,char *name) {
	LockGuard<SpinLock> guard(&_lock);
	MntSpace *ms = create(pid,_node,name);
	if(!ms)
		return NULL;

	if(ms->_tree.replaceWith(_tree) != 0) {
		release(ms);
		return NULL;
	}
	return ms;
}

ino_t MntSpace::request(const char *path,const char **end,OpenFile **file) {
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

void MntSpace::release(OpenFile *file) {
	file->decUsages();
}

int MntSpace::mount(Proc *,const char *path,OpenFile *file) {
	file->incRefs();
	LockGuard<SpinLock> guard(&_lock);
	return _tree.insert(path,file);
}

int MntSpace::remount(Proc *p,const char *path,OpenFile *dir,uint flags) {
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
	if(match->devno == VFS_DEV_NO)
		VFSNode::request(match->root);
	return 0;
}

int MntSpace::unmount(Proc *p,const char *path) {
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
		if(match->devno == VFS_DEV_NO)
			VFSNode::release(VFSNode::get(match->root));
		/* remove the mountpoint */
		file = _tree.remove(path);
		assert(file != NULL);
	}

	file->close(p->getPid());
	return 0;
}

void MntSpace::join(Proc *p) {
	if(p->ms)
		p->ms->leave(p);
	{
		LockGuard<SpinLock> guard(&_tableLock);
		_refs++;
	}
	p->ms = this;
}

void MntSpace::leave(Proc *p) {
	release(this);
	p->ms = NULL;
}

void MntSpace::print(OStream &os) const {
	LockGuard<SpinLock> guard(&_lock);
	_tree.print(os);
}
