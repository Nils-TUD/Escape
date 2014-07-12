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
#include <task/mountspace.h>
#include <task/proc.h>
#include <util.h>

SpinLock MountSpace::lock;
int MountSpace::next_id = 0;
SList<MountSpace> MountSpace::list;

int MountSpace::getId(Proc *p) {
	LockGuard<SpinLock> guard(&lock);
	return p->mounts->id;
}

int MountSpace::request(Proc *p,const char *path,const char **end,OpenFile **file) {
	{
		LockGuard<SpinLock> guard(&lock);
		PathTreeItem<OpenFile> *match = p->mounts->tree.find(path,end);
		if(!match)
			return -ENOENT;
		*file = match->getData();
	}
	if(!IS_NODE(*file))
		(*file)->incUsages();
	return 0;
}

void MountSpace::release(OpenFile *file) {
	if(!IS_NODE(file))
		file->decUsages();
}

int MountSpace::mount(Proc *p,const char *path,OpenFile *file) {
	if(!IS_NODE(file))
		file->incRefs();
	LockGuard<SpinLock> guard(&lock);
	return p->mounts->tree.insert(path,file);
}

int MountSpace::unmount(Proc *p,const char *path) {
	OpenFile *file;
	{
		LockGuard<SpinLock> guard(&lock);
		file = p->mounts->tree.remove(path);
	}
	if(file) {
		if(!IS_NODE(file))
			file->close(p->getPid());
		return 0;
	}
	return -ENOENT;
}

void MountSpace::create(Proc *p) {
	assert(p->getPid() == 0);
	LockGuard<SpinLock> guard(&lock);
	p->mounts = new MountSpace();
	if(p->mounts == NULL)
		Util::panic("Unable to create initial mountspace");
	list.append(p->mounts);
}

int MountSpace::clone(Proc *p) {
	LockGuard<SpinLock> guard(&lock);
	MountSpace *ms = new MountSpace();
	if(ms == NULL)
		return -ENOMEM;

	int res = ms->tree.replaceWith(p->mounts->tree);
	if(res == 0) {
		doLeave(p);
		p->mounts = ms;
		list.append(ms);
		res = ms->id;
	}
	else
		delete ms;
	return res;
}

void MountSpace::inherit(Proc *dst,Proc *src) {
	LockGuard<SpinLock> guard(&lock);
	dst->mounts = src->mounts;
	dst->mounts->refs++;
}

int MountSpace::join(Proc *p,int id) {
	LockGuard<SpinLock> guard(&lock);
	for(auto it = list.begin(); it != list.end(); ++it) {
		if(it->id == id) {
			doLeave(p);
			p->mounts = &*it;
			it->refs++;
			return 0;
		}
	}
	return -ENOTFOUND;
}

void MountSpace::doLeave(Proc *p) {
	if(--p->mounts->refs == 0) {
		list.remove(p->mounts);
		delete p->mounts;
	}
	p->mounts = NULL;
}

void MountSpace::printItem(OStream &os,OpenFile *file) {
	if(IS_NODE(file)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(file);
		os.writef("%s",node->getPath());
	}
	else
		os.writef("%s",file->getPath());
}

void MountSpace::printAll(OStream &os) {
	LockGuard<SpinLock> guard(&lock);
	for(auto it = list.cbegin(); it != list.cend(); ++it) {
		os.writef("MountSpace %d (%d refs):\n",it->id,it->refs);
		it->tree.print(os,printItem);
	}
}

void MountSpace::print(OStream &os,Proc *p) {
	LockGuard<SpinLock> guard(&lock);
	MountSpace *ms = p->mounts;
	os.writef("Id: %d\n",ms->id);
	os.writef("Refs: %d\n",ms->refs);
	os.writef("Tree:\n");
	ms->tree.print(os,printItem);
}
