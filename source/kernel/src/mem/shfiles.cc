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

#include <mem/region.h>
#include <mem/shfiles.h>
#include <mem/virtmem.h>
#include <vfs/openfile.h>
#include <common.h>

esc::Treap<ShFiles::FileNode> ShFiles::tree;
SpinLock ShFiles::lock;

int ShFiles::add(VMRegion *vmreg,pid_t pid) {
	OpenFile *f = vmreg->reg->getFile();
	if(f) {
		FileId fid(f->getDev(),f->getNodeNo());
		/* create an entry for us */
		FileUsage *fuse = new FileUsage(pid,vmreg->virt());
		if(!fuse)
			return -ENOMEM;

		{
			LockGuard<SpinLock> g(&lock);
			/* does anybody else have this file already? */
			FileNode *fn = tree.find(fid);
			if(!fn) {
				/* ok, create a new node for it */
				fn = new FileNode(fid);
				if(!fn) {
					delete fuse;
					return -ENOMEM;
				}
				tree.insert(fn);
			}
			/* append us to users */
			fn->usages.append(fuse);
		}

		vmreg->fileuse = fuse;
	}
	return 0;
}

bool ShFiles::get(OpenFile *f,uintptr_t *addr,pid_t *pid) {
	FileId fid(f->getDev(),f->getNodeNo());
	LockGuard<SpinLock> g(&lock);
	FileNode *fn = tree.find(fid);
	if(fn) {
		FileUsage *fuse = &*fn->usages.begin();
		*addr = fuse->addr;
		*pid = fuse->pid;
		return true;
	}
	return false;
}

void ShFiles::remove(VMRegion *vmreg) {
	if(vmreg->fileuse != NULL) {
		OpenFile *f = vmreg->reg->getFile();
		FileId fid(f->getDev(),f->getNodeNo());

		{
			LockGuard<SpinLock> g(&lock);
			/* find our file; it has to exist because we're a user of it */
			FileNode *fn = tree.find(fid);
			assert(fn);
			/* remove us */
			fn->usages.remove(vmreg->fileuse);
			/* remove the entire shared-file if there are no other users */
			if(fn->usages.length() == 0) {
				tree.remove(fn);
				delete fn;
			}
		}

		delete vmreg->fileuse;
		vmreg->fileuse = NULL;
	}
}
