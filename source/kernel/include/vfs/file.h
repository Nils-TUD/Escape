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

#pragma once

#include <vfs/node.h>
#include <common.h>
#include <mutex.h>

class VFSFile : public VFSNode {
	static const size_t DIRECT_COUNT	= 4;
	/* limit the doubly indirect entries to limit total file size (16 * 1024 * 4 KiB) */
	static const size_t DBL_INDIR_COUNT	= 16;

public:
	/**
	 * Creates a new file with name <name> in <parent>
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param mode the mode to set
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSFile(pid_t pid,VFSNode *parent,char *name,uint mode,bool &success);

	/**
	 * Creates a new file for the memory <data>..<data>+<len>.
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param data the data to make available with that file
	 * @param len the length of the data in bytes
	 * @param mode the mode to set
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSFile(pid_t pid,VFSNode *parent,char *name,void *data,size_t len,uint mode,bool &success);

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
	int reserve(off_t newSize);

	virtual ssize_t getSize(pid_t pid);
	virtual off_t seek(pid_t pid,off_t position,off_t offset,uint whence) const;
	virtual ssize_t read(pid_t pid,OpenFile *file,void *buffer,off_t offset,size_t count);
	virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count);
	virtual int truncate(pid_t pid,off_t length);

	virtual void print(OStream &os) const;

private:
	void *getIndirBlock(void ***table,off_t offset,size_t size,bool alloc);
	void *getBlock(off_t offset,bool alloc);

	bool dynamic;
	/* total size */
	off_t size;
	union {
		/* if dynamic is true */
		struct {
			void *direct[DIRECT_COUNT];
			void **indirect;
			void ***dblindir;
		};
		/* and false */
		void *data;
	};
	SpinLock lock;
};
