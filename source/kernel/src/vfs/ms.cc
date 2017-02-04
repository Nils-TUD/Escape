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

VFSMS::MSTreeItem::MSTreeItem(const VFSMS::MSTreeItem &i) : esc::PathTreeItem<OpenFile>(i) {
	if(getData() && !IS_NODE(getData()))
		getData()->incRefs();
}

int VFSMS::request(const char *path,const char **end,OpenFile **file) {
	{
		LockGuard<SpinLock> guard(&lock);
		MSTreeItem *match = _tree.find(path,end);
		if(!match)
			return -ENOENT;
		*file = match->getData();
	}
	if(!IS_NODE(*file))
		(*file)->incUsages();
	return 0;
}

void VFSMS::release(OpenFile *file) {
	if(!IS_NODE(file))
		file->decUsages();
}

int VFSMS::mount(Proc *,const char *path,OpenFile *file) {
	if(!IS_NODE(file))
		file->incRefs();
	LockGuard<SpinLock> guard(&lock);
	return _tree.insert(path,file);
}

int VFSMS::unmount(Proc *p,const char *path) {
	OpenFile *file;
	{
		LockGuard<SpinLock> guard(&lock);
		file = _tree.remove(path);
	}
	if(file) {
		if(!IS_NODE(file))
			file->close(p->getPid());
		return 0;
	}
	return -ENOENT;
}

void VFSMS::join(Proc *p) {
	if(p->msnode)
		p->msnode->leave(p);
	ref();
	p->msnode = this;
}

void VFSMS::leave(Proc *p) {
	unref();
	p->msnode = NULL;
}

ssize_t VFSMS::read(pid_t pid,OpenFile *,void *buffer,off_t offset,size_t count) {
	ssize_t res = VFSInfo::readHelper(pid,this,buffer,offset,count,0,readCallback);
	acctime = Timer::getTime();
	return res;
}

void VFSMS::readCallback(VFSNode *node,size_t *dataSize,void **buffer) {
	OStringStream os;
	node->print(os);

	*buffer = os.keepString();
	*dataSize = os.getLength();
}

void VFSMS::print(OStream &os) const {
	LockGuard<SpinLock> guard(&lock);
	_tree.print(os,printItem);
}

void VFSMS::printItem(OStream &os,OpenFile *file) {
	if(IS_NODE(file)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(file);
		os.writef("%s",node->getPath());
	}
	else
		os.writef("%s",file->getPath());
}
