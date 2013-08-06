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
#include <sys/col/slist.h>

class VFSPipe : public VFSNode {
	struct PipeData : public SListItem {
		size_t length;
		off_t offset;
		uint8_t data[];
	};

public:
	/**
	 * Creates a new pipe for <pid> in <parent>
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSPipe(pid_t pid,VFSNode *parent,bool &success);

	/**
	 * Destructor
	 */
	virtual ~VFSPipe();

	virtual size_t getSize(pid_t pid) const;
	virtual ssize_t read(pid_t pid,OpenFile *file,void *buffer,off_t offset,size_t count);
	virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count);
	virtual void close(pid_t pid,OpenFile *file);

private:
	uint8_t noReader;
	/* total number of bytes available */
	size_t total;
	/* a list with PipeData */
	SList<PipeData> list;
};
