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

class VFSFile : public VFSNode {
	/* the initial size of the write-cache for file-nodes */
	static const size_t VFS_INITIAL_WRITECACHE		= 128;

public:
	/**
	 * Creates a new file with name <name> in <parent>
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSFile(pid_t pid,VFSNode *parent,char *name,bool &success);

	/**
	 * Creates a new file for the memory <data>..<data>+<len>.
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param data the data to make available with that file
	 * @param len the length of the data in bytes
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSFile(pid_t pid,VFSNode *parent,char *name,void *data,size_t len,bool &success);

	/**
	 * Destructor
	 */
	virtual ~VFSFile();

	/**
	 * This is only intended for intern usage! It reserves <size> bytes space for the file, which
	 * requires that the allocation is dynamic.
	 *
	 * @param newSize the new size
	 * @return 0 on success
	 */
	int reserve(size_t newSize);

	virtual size_t getSize(pid_t pid) const;
	virtual off_t seek(pid_t pid,off_t position,off_t offset,uint whence) const;
	virtual ssize_t read(pid_t pid,OpenFile *file,void *buffer,off_t offset,size_t count);
	virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count);

private:
	int doReserve(size_t newSize);

	bool dynamic;
	/* size of the buffer */
	size_t size;
	/* currently used size */
	off_t pos;
	void *data;
};
