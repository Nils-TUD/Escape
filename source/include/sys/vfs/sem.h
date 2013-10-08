/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#include <sys/common.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/ostringstream.h>
#include <sys/semaphore.h>

class VFSSem : public VFSNode {
public:
	/**
	 * Creates a new semaphore for <pid> in <p>
	 *
	 * @param pid the process-id
	 * @param p the parent-node
	 * @param name the name
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSSem(pid_t pid,VFSNode *p,char *name,bool &success)
			: VFSNode(pid,name,MODE_TYPE_SEM | S_IXUSR | S_IRUSR | S_IWUSR,success), sm() {
		if(!success)
			return;

		/* auto-destroy on the last close() */
		refCount--;
		append(p);
	}

	void up() {
		sm.up();
	}
	void down() {
		sm.down();
	}

	virtual size_t getSize(A_UNUSED pid_t pid) const {
		return sm.getValue();
	}
	virtual ssize_t read(pid_t pid,OpenFile *,void *buffer,off_t offset,size_t count) {
		return VFSInfo::readHelper(pid,this,buffer,offset,count,0,readCallback);
	}

private:
	static void readCallback(VFSNode *node,size_t *dataSize,void **buffer) {
		VFSSem *sem = static_cast<VFSSem*>(node);
		OStringStream os;
		os.writef("value=%zu",sem->sm.getValue());
		*buffer = os.keepString();
		*dataSize = os.getLength();
	}

	Semaphore sm;
};
