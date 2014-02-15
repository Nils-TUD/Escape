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

#include <sys/common.h>
#include <sys/task/mountspace.h>
#include <sys/task/proc.h>
#include <sys/util.h>

SpinLock MountSpace::lock;
int MountSpace::next_id = 0;
SList<MountSpace> MountSpace::list;

int MountSpace::getId(Proc *p) {
	lock.down();
	int id = p->mounts->id;
	lock.up();
	return id;
}

int MountSpace::request(Proc *p,const char *path,const char **end,OpenFile **file) {
	lock.down();
	PathTreeItem<OpenFile> *match = p->mounts->tree.find(path,end);
	if(!match) {
		lock.up();
		return -ENOENT;
	}
	*file = match->getData();
	lock.up();
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
	lock.down();
	int res = p->mounts->tree.insert(path,file);
	lock.up();
	return res;
}

int MountSpace::unmount(Proc *p,const char *path) {
	lock.down();
	OpenFile *file = p->mounts->tree.remove(path);
	lock.up();
	if(file) {
		if(!IS_NODE(file))
			file->close(p->getPid());
		return 0;
	}
	return -ENOENT;
}

void MountSpace::create(Proc *p) {
	assert(p->getPid() == 0);
	lock.down();
	p->mounts = new MountSpace();
	if(p->mounts == NULL)
		Util::panic("Unable to create initial mountspace");
	list.append(p->mounts);
	lock.up();
}

int MountSpace::clone(Proc *p) {
	lock.down();
	MountSpace *ms = new MountSpace();
	if(ms == NULL) {
		lock.up();
		return -ENOMEM;
	}

	int res = ms->tree.replaceWith(p->mounts->tree);
	if(res == 0) {
		doLeave(p);
		p->mounts = ms;
		list.append(ms);
		res = ms->id;
	}
	else
		delete ms;

	lock.up();
	return res;
}

void MountSpace::inherit(Proc *dst,Proc *src) {
	lock.down();
	dst->mounts = src->mounts;
	dst->mounts->refs++;
	lock.up();
}

int MountSpace::join(Proc *p,int id) {
	int res = -ENOENT;
	lock.down();
	for(auto it = list.begin(); it != list.end(); ++it) {
		if(it->id == id) {
			doLeave(p);
			p->mounts = &*it;
			it->refs++;
			res = 0;
			break;
		}
	}
	lock.up();
	return res;
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
	lock.down();
	for(auto it = list.cbegin(); it != list.cend(); ++it) {
		os.writef("MountSpace %d (%d refs):\n",it->id,it->refs);
		it->tree.print(os,printItem);
	}
	lock.up();
}

void MountSpace::print(OStream &os,Proc *p) {
	lock.down();
	MountSpace *ms = p->mounts;
	os.writef("Id: %d\n",ms->id);
	os.writef("Refs: %d\n",ms->refs);
	os.writef("Tree:\n");
	ms->tree.print(os,printItem);
	lock.up();
}
