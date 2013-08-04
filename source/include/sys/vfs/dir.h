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

class VFSDir : public VFSNode {
	/* VFS-directory-entry (equal to the direntry of ext2) */
	struct VFSDirEntry {
		inode_t nodeNo;
		uint16_t recLen;
		uint16_t nameLen;
		/* name follows (up to 255 bytes) */
	} A_PACKED;

public:
	/**
	 * Creates a new directory with name <name> in <parent>
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSDir(pid_t pid,VFSNode *parent,char *name,bool &success);

	virtual off_t seek(pid_t pid,off_t position,off_t offset,uint whence);
	virtual size_t getSize(pid_t pid) const;
	virtual ssize_t read(pid_t pid,OpenFile *file,USER void *buffer,off_t offset,size_t count);
};
